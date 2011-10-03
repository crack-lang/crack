// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Cacher.h"
#include "BModuleDef.h"
#include "model/Namespace.h"

#include "builder/BuilderOptions.h"
#include "builder/util/SourceDigest.h"
#include "builder/util/CacheFiles.h"

#include <assert.h>

#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/LLVMContext.h>

using namespace llvm;
using namespace llvm::sys;
using namespace builder::mvll;
using namespace std;
using namespace model;

void Cacher::writeMetadata() {

    // encode metadata into the bitcode

    vector<Value *> dList;
    NamedMDNode *node;
    Module *module = modDef->rep;

    node = module->getOrInsertNamedMetadata("crack_origin_digest");
    dList.push_back(MDString::get(getGlobalContext(), modDef->digest.asHex()));
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), 1));

    // original source path
    dList.clear();
    node = module->getOrInsertNamedMetadata("crack_origin_path");
    dList.push_back(MDString::get(getGlobalContext(), modDef->path));
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), 1));

    /*
    NamedMDNode *globals = module->getOrInsertNamedMetadata("crack_globals");

    vector<Value *> gList;
    for (Namespace::VarDefMap::const_iterator varIter = modDef->beginDefs();
         varIter != modDef->endDefs();
         ++varIter
         ) {
        gList.push_back(MDString::get(getGlobalContext(),
                                      varIter->second->name));
    }
    globals->addOperand(MDNode::get(getGlobalContext(), gList.data(), gList.size()));


    */

    //module->dump();

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

    // if cache file exists, we need to get a digest for comparison
    // if it doesn't exist, we digest it when we save
    SourceDigest digest = SourceDigest::fromFile(path);

    return NULL;
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



