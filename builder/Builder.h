
#ifndef _builder_Builder_h_
#define _builder_Builder_h_

#include <spug/RCPtr.h>

#include "model/FuncCall.h" // for FuncCall::ExprVector

namespace model {
    class Context;
    SPUG_RCPTR(FuncCall);
    SPUG_RCPTR(FuncDef);
    SPUG_RCPTR(StrConst);
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

        virtual model::FuncDefPtr createFuncDef(const char *name) = 0;
        virtual model::FuncCallPtr 
            createFuncCall(const std::string &funcName) = 0;
        virtual void createModule(const char *name) = 0;
        virtual void closeModule() = 0;
        virtual model::StrConstPtr createStrConst(const std::string &val) = 0;
        
        virtual void registerPrimFuncs(model::Context &context) = 0;
        
        virtual void run() = 0;
};

} // namespace builder

#endif

