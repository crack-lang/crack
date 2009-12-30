
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
        typedef std::vector<ExprPtr> ExprVec;
        ExprVec args;
        
        // if FuncCall is a method, this is the receiver (the "this"), 
        // otherwise it will be null
        ExprPtr receiver;

        FuncCall(FuncDef *funcDef);
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};


} // namespace model

#endif
