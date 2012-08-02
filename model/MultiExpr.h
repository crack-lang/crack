// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_MultiExpr_h_
#define _model_MultiExpr_h_

#include <vector>

#include "Expr.h"

namespace model {

SPUG_RCPTR(FuncCall);

SPUG_RCPTR(MultiExpr);

/**
 * This is a high-level expression type that is a list of expression that are 
 * evaluated sequentially.  The result of a MultiExpr is the result of the 
 * last sub-expression.  A MultiExpr must have at least one sub-expression.
 */
class MultiExpr : public Expr {
    private:
        std::vector<ExprPtr> elems;

    public:

        MultiExpr() : Expr(0) {}

        void add(Expr *expr) { elems.push_back(expr); }

        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isProductive() const;
};

} // namespace model

#endif
