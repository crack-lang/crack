// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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

