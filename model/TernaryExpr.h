// Copyright 2009 Google Inc.

#ifndef _model_TernaryExpr_h_
#define _model_TernaryExpr_h_

#include "Expr.h"

namespace model {

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
};

} // namespace model

#endif
