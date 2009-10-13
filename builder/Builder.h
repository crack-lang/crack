
#ifndef _builder_Builder_h_
#define _builder_Builder_h_

#include <spug/RCPtr.h>

#include "model/FuncCall.h" // for FuncCall::ExprVector

namespace model {
    class Context;
    class IntConst;
    SPUG_RCPTR(FuncCall);
    SPUG_RCPTR(FuncDef);
    SPUG_RCPTR(IntConst);
    SPUG_RCPTR(StrConst);
    SPUG_RCPTR(TypeDef);
    SPUG_RCPTR(VarDef);
    SPUG_RCPTR(VarRef);
    class FuncCall;
};

namespace builder {

/** Abstract base class for builders.  Builders generate code. */
class Builder {
    public:
        virtual void emitFuncCall(model::Context &context,
                                  const model::FuncDefPtr &func,
                                  const model::FuncCall::ExprVector &args
                                  ) = 0;
        
        virtual void emitStrConst(model::Context &context,
                                  const model::StrConstPtr &strConst
                                  ) = 0;

        virtual void emitIntConst(model::Context &context,
                                  const model::IntConst &val) = 0;

        /**
         * Emits a variable definition and returns a new VarDef object for the 
         * variable.
         * @param staticScope true if the "static" keyword was applied to the 
         *        definition.
         */
        virtual model::VarDefPtr emitVarDef(
            model::Context &container,
            const model::TypeDefPtr &type,
            const std::string &name,
            const model::ExprPtr &initializer = 0,
            bool staticScope = false
        ) = 0;
    
        virtual void emitVarRef(model::Context &context,
                                const model::VarRef &var
                                ) = 0;

        virtual model::FuncDefPtr createFuncDef(const char *name) = 0;
        virtual model::FuncCallPtr 
            createFuncCall(const std::string &funcName) = 0;
        virtual model::VarRefPtr
            createVarRef(const model::VarDefPtr &varDef) = 0;
        virtual void createModule(const char *name) = 0;
        virtual void closeModule() = 0;
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val) = 0;
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val) = 0;
        
        virtual void registerPrimFuncs(model::Context &context) = 0;
        
        virtual void run() = 0;
};

} // namespace builder

#endif

