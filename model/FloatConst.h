// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_FloatConst_h_
#define _model_FloatConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(FloatConst);

// As with integers, there's only one kind of float constant -
// it can be specialized based on context - e.g. 32 bit or 64 bit
class FloatConst : public Expr {
    public:
        double val;

        FloatConst(TypeDef *type, double val);
        
        virtual ResultExprPtr emit(Context &context);
        virtual ExprPtr convert(Context &context, TypeDef *newType);
        virtual void writeTo(std::ostream &out) const;        
        bool isAdaptive() const;

        virtual FloatConstPtr create(double val) const;
        
        ExprPtr foldFAdd(Expr *other);
        ExprPtr foldFSub(Expr *other);
        ExprPtr foldFMul(Expr *other);
        ExprPtr foldFDiv(Expr *other);
        ExprPtr foldFRem(Expr *other);
        ExprPtr foldNeg();
};

} // namespace parser

#endif
