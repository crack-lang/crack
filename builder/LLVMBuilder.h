
#ifndef _builder_LLVMBUilder_h_
#define _builder_LLVMBUilder_h_

#include "Builder.h"

namespace llvm {
    class Module;
    class Function;
    class BasicBlock;
    class Value;
    class Function;
    class ExecutionEngine;
};

namespace builder {

class LLVMBuilder : public Builder {
    private:

        llvm::Module *module;
        llvm::Function *func;
        llvm::BasicBlock *block;
        
        llvm::Value *lastValue;
        llvm::ExecutionEngine *execEng;

    public:
        LLVMBuilder();

        virtual void emitFuncCall(model::Context &context, 
                                  const model::FuncDefPtr &func,
                                  const model::FuncCall::ExprVector &args
                                  );
        
        virtual void emitStrConst(model::Context &context,
                                  const model::StrConstPtr &strConst
                                  );
        
        // for definitions, we're going to just let the builder be a factory 
        // so that it can tie whatever special information it wants to the 
        // definition.
        
        virtual model::FuncCallPtr createFuncCall(const std::string &funcName);
        virtual model::FuncDefPtr createFuncDef(const char *name);

        virtual void createModule(const char *name);
        virtual void closeModule();
        virtual model::StrConstPtr createStrConst(const std::string &val);
        virtual void registerPrimFuncs(model::Context &context);
        
        virtual void run();
        
        void kludge(model::Context &context);
};

} // namespace builder
#endif

