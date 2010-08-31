// Copyright 2009 Google Inc.

#ifndef _model_IntConst_h_
#define _model_IntConst_h_

#include "Context.h"
#include "Expr.h"
#include <stdint.h>

namespace model {

SPUG_RCPTR(IntConst);

// There's only one kind of integer constant - it can be specialized based on 
// context - e.g. 32 bit, 64 bit, signed versus unsigned...
class IntConst : public Expr {
    public:
        int64_t val;

        IntConst(TypeDef *type, int64_t val);
        
        virtual ResultExprPtr emit(Context &context);
        virtual ExprPtr convert(Context &context, TypeDef *newType);
        virtual void writeTo(std::ostream &out) const;
        virtual bool isAdaptive() const;
};

} // namespace parser

#endif
