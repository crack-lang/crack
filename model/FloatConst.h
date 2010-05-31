// Copyright 2009 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

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
};

} // namespace parser

#endif
