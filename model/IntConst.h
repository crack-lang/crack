// Copyright 2009-2011 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_IntConst_h_
#define _model_IntConst_h_

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(IntConst);

// There's only one kind of integer constant - it can be specialized based on 
// context - e.g. 32 bit, 64 bit, signed versus unsigned...
class IntConst : public Expr {
    public:

        union {
             int64_t sval;
            uint64_t uval;
        } val;

        bool reqUnsigned; // > INT64_MAX so requires unsigned 64
        bool uneg; // "unsigned negative"

        IntConst(TypeDef *type, int64_t val);
        IntConst(TypeDef *type, uint64_t val);
        
        virtual ResultExprPtr emit(Context &context);
        virtual ExprPtr convert(Context &context, TypeDef *newType);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isAdaptive() const;
        
        virtual IntConstPtr create(int64_t v);
        virtual IntConstPtr create(uint64_t v);
        
        /** Return the default type for the value. */
        static TypeDef *selectType(Context &context, int64_t val);

        ExprPtr foldAdd(Expr *other);
        ExprPtr foldSub(Expr *other);
        ExprPtr foldMul(Expr *other);
        ExprPtr foldSDiv(Expr *other);
        ExprPtr foldUDiv(Expr *other);
        ExprPtr foldSRem(Expr *other);
        ExprPtr foldURem(Expr *other);
        ExprPtr foldOr(Expr *other);
        ExprPtr foldAnd(Expr *other);
        ExprPtr foldXor(Expr *other);
        ExprPtr foldShl(Expr *other);
        ExprPtr foldLShr(Expr *other);
        ExprPtr foldAShr(Expr *other);
        ExprPtr foldNeg();
        ExprPtr foldBitNot();
};

} // namespace parser

#endif
