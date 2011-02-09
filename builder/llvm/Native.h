// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Native_h_
#define _builder_llvm_Native_h_

#include <vector>
#include <string>

namespace llvm {
    class Module;
}

namespace builder {

class BuilderOptions;

namespace mvll {

void nativeCompile(llvm::Module *module,
                   const builder::BuilderOptions *o,
                   const std::vector<std::string> &sharedLibs,
                   const std::vector<std::string> &libPaths);

} // end namespace builder::vmll
} // end namespace builder

#endif
