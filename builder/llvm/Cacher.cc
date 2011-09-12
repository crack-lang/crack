// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Cacher.h"

#include "builder/BuilderOptions.h"

#include <assert.h>

#include <llvm/Support/Program.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Bitcode/ReaderWriter.h>

using namespace llvm;
using namespace llvm::sys;
using namespace builder::mvll;
using namespace std;

string Cacher::getCacheFilePath(const std::string &canonicalName) {


    sys::Path path;

    // look for an explicit cache path
    BuilderOptions::StringMap::const_iterator i = options->optionMap.find("cachePath");
    if (i == options->optionMap.end()) {
        // no explicit cache path
        // find the first writable path in library paths
        for (int i = 0; i < context.construct->sourceLibPath.size(); i++) {
            path = context.construct->sourceLibPath[i];
            if (path.canWrite())
                break;
        }
    }
    else {
        // explicit path
        path = i->second;
    }

    // if we don't have a valid, writable path, we're done
    if (!path.canWrite() || !path.isValid())
        return "";

    // XXX save the root cache path somewhere static, so we don't find
    // it for each file we need to cache!

    // now add/create the paths inside of the root cache directory that correspond to
    // the canonicalName
    string canonicalDir = canonicalName;
    for (int i = 0; i < canonicalDir.size(); i++) {
        if (canonicalDir[i] == '.')
            canonicalDir[i] = '/';
    }
    path.appendComponent(canonicalDir);
    path.appendSuffix("bc");

    sys::Path dpath = path;
    dpath.eraseComponent();
    //dpath.createDirectoryOnDisk(true);

    //path.createFileOnDisk();

    return path.str();

}

void Cacher::writeBitcode(llvm::Module *module) {

    /*
    BuilderOptions::StringMap::const_iterator i = options->optionMap.find("out");
    assert(i != options->optionMap.end() && "no out");

    sys::Path oFile(i->second);
    sys::Path binFile(i->second);

    oFile.eraseSuffix();
    binFile.eraseSuffix();

    std::string error;
    unsigned OpenFlags = 0;
    OpenFlags |= raw_fd_ostream::F_Binary;
    tool_output_file *FDOut = new tool_output_file(oFile.str().c_str(),
                                                   Err,
                                                   OpenFlags);
    if (!Err.empty()) {
        cerr << error << '\n';
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
    */

}

BModuleDefPtr Cacher::maybeLoadFromCache(const string &canonicalName,
                                         const string &path) {
    if (options->verbosity >= 2)
        cerr << "attempting cache load: " << canonicalName << ", " << path << endl;

    string cacheFile = getCacheFilePath(canonicalName);
    if (cacheFile.empty())
        return NULL;

    return NULL;
}

void Cacher::saveToCache() {

    assert(modDef && "empty modDef for saveToCache");

    string cacheFile = getCacheFilePath(modDef->getFullName());
    if (cacheFile.empty()) {
        if (options->verbosity >= 1)
            cerr << "unable to find writeable directory for cache: caching skipped" << endl;
        return;
    }

    if (options->verbosity >= 2)
        cerr << "caching " << modDef->getFullName() << " to file: " << cacheFile << endl;

}



