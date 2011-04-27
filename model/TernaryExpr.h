// Copyright 2009 Google Inc.

#ifndef _model_TernaryExpr_h_
#define _model_TernaryExpr_h_

#include "Expr.h"

namespace model {

/**
 * A ternary expression.
 * As a special case to accomodate void-typed interpolated strings, the 
 * "false" expression pointer may be null.
 */
class TernaryExpr : public Expr {
    public:
        ExprPtr cond, trueVal, falseVal;
        TernaryExpr(Expr *cond, Expr *trueVal, Expr *falseVal,
                    TypeDef *type
                    ) : 
            Expr(type),
            cond(cond),
            trueVal(trueVal),
            falseVal(falseVal) {
        }

        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isProductive() const;
};

} // namespace model

#endif
