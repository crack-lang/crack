// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "Cacher.h"

#include "builder/BuilderOptions.h"

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
    return NULL;
}

void Cacher::saveToCache() {

}



