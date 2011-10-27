// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Cacher.h"
#include "BModuleDef.h"
#include "model/Namespace.h"

#include "builder/llvm/LLVMBuilder.h"
#include "builder/BuilderOptions.h"
#include "builder/util/CacheFiles.h"
#include "builder/util/SourceDigest.h"

#include <assert.h>
#include <vector>

#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/system_error.h>
#include <llvm/LLVMContext.h>

using namespace llvm;
using namespace llvm::sys;
using namespace builder::mvll;
using namespace std;
using namespace model;

// metadata version
const std::string Cacher::MD_VERSION = "1";

void Cacher::addNamedStringNode(const string &key, const string &val) {

    vector<Value *> dList;
    NamedMDNode *node;

    node = modDef->rep->getOrInsertNamedMetadata(key);
    dList.push_back(MDString::get(getGlobalContext(), val));
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), 1));

}

string Cacher::getNamedStringNode(const std::string &key) {

    NamedMDNode *node;
    MDNode *mnode;
    MDString *str;

    node = modDef->rep->getNamedMetadata(key);
    assert(node && "missing required named string node");
    mnode = node->getOperand(0);
    assert(mnode && "malformed string node 1");
    str = dyn_cast<MDString>(mnode->getOperand(0));
    assert(str && "malformed string node 2");
    return str->getString().str();

}

void Cacher::writeMetadata() {

    // encode metadata into the bitcode
    addNamedStringNode("crack_md_version", Cacher::MD_VERSION);
    addNamedStringNode("crack_origin_digest", modDef->digest.asHex());
    addNamedStringNode("crack_origin_path", modDef->path);

    vector<Value *> dList;
    NamedMDNode *node;
    Module *module = modDef->rep;

    // crack_imports: operand list points to import nodes
    // import node: 0: canonical name 1: source digest
    node = module->getOrInsertNamedMetadata("crack_imports");
    for (vector<BModuleDef*>::const_iterator varIter = modDef->importList.begin();
         varIter != modDef->importList.end();
         ++varIter
         ) {
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*varIter)->getFullName()));
        dList.push_back(MDString::get(getGlobalContext(),
                                      (*varIter)->digest.asHex()));
        node->addOperand(MDNode::get(getGlobalContext(), dList.data(), dList.size()));
        dList.clear();
    }

    // crack_externs: these we need to resolve upon load. in the JIT, that means
    // global mappings
    node = module->getOrInsertNamedMetadata("crack_externs");
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    for (LLVMBuilder::ModFuncMap::const_iterator i = b.moduleFuncs.begin();
         i != b.moduleFuncs.end();
         ++i) {
        // only include it if it's a decl and it's defined in another module
        // but skip externals from extensions, since these are found by
        // the jit through symbol searching the process
        ModuleDefPtr owner = i->first->getOwner()->getModule();
        if ((!i->second->isDeclaration()) ||
            (owner && owner->fromExtension) ||
            (i->first->getOwner() == modDef->getParent(0).get()))
            continue;
        dList.push_back(MDString::get(getGlobalContext(), i->second->getNameStr()));
    }
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), dList.size()));

    //module->dump();

}

bool Cacher::doImports() {

    MDNode *mnode;
    MDString *cname, *digest;
    SourceDigest iDigest;
    BModuleDefPtr m;
    NamedMDNode *imports = modDef->rep->getNamedMetadata("crack_imports");

    assert(imports && "missing crack_imports node");

    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // canonical name
        cname = dyn_cast<MDString>(mnode->getOperand(0));
        assert(cname && "malformed import node: canonical name");

        // source digest
        digest = dyn_cast<MDString>(mnode->getOperand(1));
        assert(digest && "malformed import node: digest");

        iDigest = SourceDigest::fromHex(digest->getString().str());

        // load this module. if the digest doesn't match, we miss
        m = context.construct->loadModule(cname->getString().str());
        if (m->digest != iDigest)
            return false;

    }

    return true;

}

bool Cacher::readMetadata() {

    string snode;

    // first check metadata version
    snode = getNamedStringNode("crack_md_version");
    if (snode != Cacher::MD_VERSION)
        return false;

    // compare the digest stored in the bitcode against the current
    // digest of the source file on disk. if they don't match, we miss
    snode = getNamedStringNode("crack_origin_digest");
    SourceDigest bcDigest = SourceDigest::fromHex(snode);
    if (bcDigest != modDef->digest)
        return false;

    modDef->path = getNamedStringNode("crack_origin_path");

    // import list
    if (!doImports())
        return false;

    // cache hit
    return true;

}

void Cacher::writeBitcode(const string &path) {

    Module *module = modDef->rep;

    std::string Err;
    unsigned OpenFlags = 0;
    OpenFlags |= raw_fd_ostream::F_Binary;

    tool_output_file *FDOut = new tool_output_file(path.c_str(),
                                                   Err,
                                                   OpenFlags);
    if (!Err.empty()) {
        cerr << Err << '\n';
        delete FDOut;
        return;
    }

    {
        formatted_raw_ostream FOS(FDOut->os());
        // llvm bitcode
        WriteBitcodeToFile(module, FOS);
    }

    // note FOS needs to destruct before we can keep
    FDOut->keep();
    delete FDOut;

}

BModuleDef *Cacher::maybeLoadFromCache(const string &canonicalName,
                                       const string &path) {

    string cacheFile = getCacheFilePath(context, options, canonicalName, "bc");
    if (cacheFile.empty())
        return NULL;

    if (options->verbosity >= 2)
        cerr << "attempting to load " << canonicalName << " from file: "
             << cacheFile << endl;

    OwningPtr<MemoryBuffer> fileBuf;
    if (error_code ec = MemoryBuffer::getFile(cacheFile.c_str(), fileBuf)) {
        if (options->verbosity >= 2)
            cerr << "getFile: " << ec.message() << endl;
        return NULL;
    }

    string errMsg;
    Module *module = getLazyBitcodeModule(fileBuf.take(), getGlobalContext(), &errMsg);
    if (!module) {
        fileBuf.reset();
        if (options->verbosity >= 1)
            cerr << "failed to load bitcode: " << errMsg << endl;
        return NULL;
    }

    // if we get here, we've loaded bitcode successfully
    modDef = new BModuleDef(canonicalName, context.ns.get(), module);

    // if cache file exists, we need to get a digest for comparison
    // if it doesn't exist, we digest it when we save
    modDef->digest = SourceDigest::fromFile(path);

    if (readMetadata()) {
        // cache hit
        return modDef;
    }
    else {
        // during meta data read, we determined we will miss
        delete modDef;
        delete module;
        return NULL;
    }

}

void Cacher::saveToCache() {

    assert(modDef && "empty modDef for saveToCache");
    assert(!modDef->path.empty() && "module source path not set");

    string cacheFile = getCacheFilePath(context, options, modDef->getFullName(), "bc");
    if (cacheFile.empty()) {
        if (options->verbosity >= 1)
            cerr << "unable to find writeable directory for cache: caching skipped"
                 << endl;
        return;
    }

    if (options->verbosity >= 2)
        cerr << "caching " << modDef->getFullName() << " from " << modDef->path
             << " to file: " << cacheFile << endl;

    // digest the source file
    modDef->digest = SourceDigest::fromFile(modDef->path);

    writeMetadata();
    writeBitcode(cacheFile);

}



