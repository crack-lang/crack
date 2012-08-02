// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_NullConst_h_
#define _model_NullConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(NullConst);

// A null constant.  Like IntConst, this will morph to adapt to the 
// corresponding type/width etc.
class NullConst : public Expr {
    public:
        NullConst(TypeDef *type) : Expr(type) {}
        
        virtual ResultExprPtr emit(Context &context);
        
        virtual ExprPtr convert(Context &context, TypeDef *newType);
        virtual void writeTo(std::ostream &out) const;        
        virtual bool isProductive() const;
};

} // namespace parser

#endif
