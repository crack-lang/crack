// Copyright 2011 Google Inc.

#ifndef _model_GetRegisterExpr_h_
#define _model_GetRegisterExpr_h_

#include "Expr.h"

namespace model {
    
/** 
 * Expression that obtains the value of the register in the current context.
 */
class GetRegisterExpr : public Expr {
    public:
        ExprPtr val;
        GetRegisterExpr(TypeDef *type) : Expr(type) {}
        
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isProductive() const;
};

} // namespace model

#endif

