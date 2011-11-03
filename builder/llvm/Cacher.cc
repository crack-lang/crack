// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Cacher.h"
#include "BModuleDef.h"
#include "model/Namespace.h"
#include "model/OverloadDef.h"

#include "builder/llvm/LLVMBuilder.h"
#include "builder/BuilderOptions.h"
#include "builder/llvm/VarDefs.h"
#include "builder/llvm/BFuncDef.h"
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

namespace {
    ConstantInt *constInt(int c) {
        return ConstantInt::get(Type::getInt32Ty(getGlobalContext()), c);
    }
}

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
        assert(i->first->getOwner() && "no owner");
        ModuleDefPtr owner = i->first->getOwner()->getModule();
        if ((!i->second->isDeclaration()) ||
            (owner && owner->fromExtension) ||
            (i->first->getOwner() == modDef->getParent(0).get()))
            continue;
        dList.push_back(MDString::get(getGlobalContext(), i->second->getNameStr()));
    }
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), dList.size()));
    dList.clear();

    // crack_defs: the symbols defined in this module that we need to rebuild
    // at compile time in order to use this cached module to compile fresh code
    // from
    node = module->getOrInsertNamedMetadata("crack_defs");
    OverloadDef *ol;
    Namespace *mns = static_cast<Namespace*>(modDef);
    for (ModuleDef::VarDefMap::const_iterator i = modDef->beginDefs();
         i != modDef->endDefs();
         ++i) {
        if (ol = dynamic_cast<OverloadDef*>(i->second.get())) {
            for (OverloadDef::FuncList::const_iterator f = ol->beginTopFuncs();
                 f != ol->endTopFuncs();
                 ++f) {
                // skip aliases
                if ((*f)->getOwner() == mns)
                    node->addOperand(writeFuncDef((*f).get()));
            }
        }
        else {
            // skip aliases
            if (i->second->getOwner() == mns)
                node->addOperand(writeVarDef(i->second.get()));
        }
    }

    //module->dump();

}

MDNode *Cacher::writeFuncDef(FuncDef *sym) {

    vector<Value *> dList;

    BFuncDef *bf = dynamic_cast<BFuncDef *>(sym);
    assert(bf && "not BFuncDef");

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), sym->name));

    // operand 1: symbol type
    dList.push_back(constInt(Cacher::function));

    // operand 2: llvm rep
    dList.push_back(bf->rep);

    // operand 3: funcdef flags
    dList.push_back(constInt(bf->flags));

    // operand 4: return type
    dList.push_back(MDString::get(getGlobalContext(), bf->returnType->name));

    // operand 5..ARITY: pairs of parameter symbol names and their types
    for (FuncDef::ArgVec::const_iterator i = bf->args.begin();
         i != bf->args.end();
         ++i) {
        dList.push_back(MDString::get(getGlobalContext(), (*i)->name));
        dList.push_back(MDString::get(getGlobalContext(), (*i)->type->name));
    }

    return MDNode::get(getGlobalContext(), dList.data(), dList.size());

}

MDNode *Cacher::writeVarDef(VarDef *sym) {

    vector<Value *> dList;

    BGlobalVarDefImpl *bvar = dynamic_cast<BGlobalVarDefImpl *>(sym->impl.get());
    assert(bvar && "not BGlobalVarDefImpl");

    // operand 0: symbol name (not canonical)
    dList.push_back(MDString::get(getGlobalContext(), sym->name));

    // operand 1: symbol type
    dList.push_back(constInt(Cacher::global));

    // operand 2: llvm rep
    dList.push_back(bvar->rep);

    return MDNode::get(getGlobalContext(), dList.data(), dList.size());

}

bool Cacher::readImports() {

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

void Cacher::readFuncDef(const std::string &sym,
                         llvm::Value *rep,
                         llvm::MDNode *mnode) {

    // operand 3: func flags
    ConstantInt *flags = dyn_cast<ConstantInt>(mnode->getOperand(3));
    assert(flags && "malformed def node: function flags");

    // operand 4: return type
    MDString *rtStr = dyn_cast<MDString>(mnode->getOperand(4));
    assert(rtStr && "malformed def node: function return type");

    // llvm function
    Function *f = dyn_cast<Function>(rep);
    assert(f && "malformed def node: llvm rep not function");

    // model funcdef
    BFuncDef *newF = new BFuncDef((FuncDef::Flags)flags->getLimitedValue(),
                                  sym,
                                  f->getArgumentList().size());
    newF->rep = f;
    newF->setOwner(modDef);

    VarDefPtr vd = context.ns->lookUp(rtStr->getString().str());
    TypeDef *td = TypeDefPtr::rcast(vd);
    assert(td && "return type not found");

    newF->returnType = td;

    if (mnode->getNumOperands() > 4) {

        MDString *aSym, *aTypeStr;
        VarDefPtr aTypeV;
        TypeDef *aType;

        // operand 5..arity: function parameter names and types
        for (int i = 5, ai=0; i < mnode->getNumOperands(); i+=2, ++ai) {

            aSym = dyn_cast<MDString>(mnode->getOperand(i));
            assert(aSym && "function arg: missing symbol");
            aTypeStr = dyn_cast<MDString>(mnode->getOperand(i+1));
            assert(aTypeStr && "function arg: missing type");
            aTypeV = context.ns->lookUp(aTypeStr->getString().str());
            aType = TypeDefPtr::rcast(aTypeV);
            assert(aType && "function arg: type not found");
            newF->args[ai] = new ArgDef(aType, aSym->getString().str());

        }
    }

    OverloadDef *o;
    vd = modDef->lookUp(sym);
    if (!vd) {
        o = new OverloadDef(sym);
        o->addFunc(newF);
        modDef->addDef(o);
    }
    else {
        o = OverloadDefPtr::rcast(vd);
        assert(o && "not an overload");
        o->addFunc(newF);
    }


}

void Cacher::readDefs() {

    MDNode *mnode;
    MDString *mstr;
    string sym;
    Value *rep;
    NamedMDNode *imports = modDef->rep->getNamedMetadata("crack_defs");

    assert(imports && "missing crack_defs node");

    for (int i = 0; i < imports->getNumOperands(); ++i) {

        mnode = imports->getOperand(i);

        // operand 0: symbol name
        mstr = dyn_cast<MDString>(mnode->getOperand(0));
        assert(mstr && "malformed def node: symbol name");
        sym = mstr->getString().str();

        // operand 1: symbol type
        ConstantInt *type = dyn_cast<ConstantInt>(mnode->getOperand(1));
        assert(type && "malformed def node: symbol type");

        // operand 2: llvm rep
        rep = mnode->getOperand(2);
        assert(rep && "malformed def node: llvm rep");

        switch (type->getLimitedValue()) {
        case Cacher::function:
            readFuncDef(sym, rep, mnode);
            break;
        case Cacher::global:
            // XXX
            break;

        default:
            assert(0 && "unhandled def type");
        }

    }


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
    // if readImports returns false, then one of our dependencies has
    // changed on disk and our own cache therefore fails
    if (!readImports())
        return false;

    // var defs
    readDefs();

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



