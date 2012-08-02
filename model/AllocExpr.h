// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_AllocExpr_h_
#define _model_AllocExpr_h_

#include <string>
#include <vector>
#include "Expr.h"

namespace model {

SPUG_RCPTR(AllocExpr);

class AllocExpr : public Expr {
    public:
        AllocExpr(TypeDef *type) : Expr(type) {}
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};


} // namespace model

#endif
