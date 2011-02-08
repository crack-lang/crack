// Copyright 2009 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_LLVMJitBuilder_h_
#define _builder_LLVMJitBuilder_h_

#include "LLVMBuilder.h"

namespace llvm {
    class ExecutionEngine;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(LLVMJitBuilder);

class LLVMJitBuilder : public LLVMBuilder {
    private:
        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);

    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);

        virtual void addGlobalVarMapping(llvm::GlobalValue*,
                                         llvm::GlobalValue*);

        virtual void engineBindModule(model::ModuleDef *moduleDef);
        virtual void engineFinishModule(model::ModuleDef *moduleDef);


    public:
        LLVMJitBuilder(void) : execEng(0) { }

        virtual void *getFuncAddr(llvm::Function *func);

        virtual void run();

        virtual void dump();

        virtual BuilderPtr createChildBuilder();

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name
                                                 );
        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

};

} } // namespace

#endif
