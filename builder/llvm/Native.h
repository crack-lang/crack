// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Native_h_
#define _builder_llvm_Native_h_

namespace llvm {
    class Module;
}

namespace builder {

class BuildOptions;

namespace mvll {

void nativeCompile(llvm::Module *module, const builder::BuildOptions *o);

} // end namespace builder::vmll
} // end namespace builder

#endif
