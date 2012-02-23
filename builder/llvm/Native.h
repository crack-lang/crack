// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Native_h_
#define _builder_llvm_Native_h_

#include <vector>
#include <string>

namespace llvm {
    class Module;
    class Value;
}

namespace builder {

class BuilderOptions;

namespace mvll {

// create main entry IR
void createMain(llvm::Module *mod, const BuilderOptions *o,
                llvm::Value *vtableBaseTypeBody,
                const std::string &mainModuleName
                );

// optimize a single unit (module)
void optimizeUnit(llvm::Module *module, int optimizeLevel);

// link time optimizations
void optimizeLink(llvm::Module *module, bool verify);

// generate native object file and link to create native binary
void nativeCompile(llvm::Module *module,
                   const builder::BuilderOptions *o,
                   const std::vector<std::string> &sharedLibs,
                   const std::vector<std::string> &libPaths);

} // end namespace builder::vmll
} // end namespace builder

#endif
