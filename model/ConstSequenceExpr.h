// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_ConstSequenceExpr_h_
#define _model_ConstSequenceExpr_h_

#include <vector>

#include "Expr.h"

namespace model {

SPUG_RCPTR(FuncCall);

SPUG_RCPTR(ConstSequenceExpr);

/**
 * This is a high-level expression type that holds a sequence constant.
 */
class ConstSequenceExpr : public Expr {
    public:
        FuncCallPtr container;
        std::vector<FuncCallPtr> elems;

        ConstSequenceExpr(TypeDef *type) : Expr(type) {}

        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};

} // namespace model

#endif
