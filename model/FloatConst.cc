// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "FloatConst.h"

#include <math.h>
#include "builder/Builder.h"
#include "parser/ParseError.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "IntConst.h"

using namespace model;
using namespace parser;

FloatConst::FloatConst(TypeDef *type, double val) :
    Expr(type),
    val(val) {
}

ResultExprPtr FloatConst::emit(Context &context) {
    return context.builder.emitFloatConst(context, this);
}

ExprPtr FloatConst::convert(Context &context, TypeDef *newType) {
    if (newType == this->type)
         return this;

    if (newType == context.construct->float64Type ||
        newType == context.construct->float32Type ||
        newType == context.construct->floatType
        )
        ; // nothing we can do about these right now
    else {
        // delegate all other conversions to the type
        return Expr::convert(context, newType);
    }

    return context.builder.createFloatConst(context, val, newType);

}

void FloatConst::writeTo(std::ostream &out) const {
    out << val;
}

bool FloatConst::isAdaptive() const {
    return true;
}

FloatConstPtr FloatConst::create(double val) const {
    return new FloatConst(type.get(), val);
}

ExprPtr FloatConst::foldFRem(Expr *other) {
    FloatConstPtr fo = FloatConstPtr::cast(other);
    if (fo)
        return create(val - floor(val / fo->val) * fo->val);

    return 0;
}

ExprPtr FloatConst::foldNeg() {
    return create(-val);
}

#define FOLD(name, sym) \
    ExprPtr FloatConst::fold##name(Expr *other) {                           \
        FloatConstPtr fo = FloatConstPtr::cast(other);                      \
        if (fo)                                                             \
            return create(val sym fo->val);                                 \
                                                                            \
        return 0;                                                           \
    }

FOLD(FAdd, +)
FOLD(FSub, -)
FOLD(FMul, *)
FOLD(FDiv, /)

