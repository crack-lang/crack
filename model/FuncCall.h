
#ifndef _model_FuncCall_h_
#define _model_FuncCall_h_

#include <string>
#include <vector>
#include "Expr.h"

namespace model {

SPUG_RCPTR(FuncDef);

SPUG_RCPTR(FuncCall);

class FuncCall : public Expr {
    public:
        FuncDefPtr func;
        typedef std::vector<ExprPtr> ExprVector;
        ExprVector args;
        
        // if FuncCall is a method, this is the receiver (the "this"), 
        // otherwise it will be null
        ExprPtr receiver;

        FuncCall(FuncDef *funcDef);
        virtual void emit(Context &context);
};


} // namespace model

#endif
