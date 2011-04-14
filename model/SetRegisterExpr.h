// Copyright 2011 Google Inc.

#ifndef _model_SetRegisterExpr_h_
#define _model_SetRegisterExpr_h_

#include "Expr.h"

namespace model {
    
/** 
 * Expression that obtains the value of the register in the current context.
 */
class SetRegisterExpr : public Expr {
    public:
        ExprPtr expr;

        SetRegisterExpr(Expr *expr) : Expr(expr->type.get()), expr(expr) {}
        
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isProductive() const;
};

} // namespace model

#endif

