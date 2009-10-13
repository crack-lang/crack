
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
        
        virtual void emitIntConst(model::Context &context,
                                  const model::IntConst &val);

        virtual model::VarDefPtr emitVarDef(model::Context &container,
                                            const model::TypeDefPtr &type,
                                            const std::string &name,
                                            const model::ExprPtr &initializer,
                                            bool staticScope
                                            );

        virtual void emitVarRef(model::Context &context,
                                const model::VarRef &var
                                );
        
        // for definitions, we're going to just let the builder be a factory 
        // so that it can tie whatever special information it wants to the 
        // definition.
        
        virtual model::FuncCallPtr createFuncCall(const std::string &funcName);
        virtual model::FuncDefPtr createFuncDef(const char *name);
        virtual model::VarRefPtr createVarRef(const model::VarDefPtr &varDef);

        virtual void createModule(const char *name);
        virtual void closeModule();
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val);
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val);
        virtual void registerPrimFuncs(model::Context &context);
        
        virtual void run();
        
};

} // namespace builder
#endif

