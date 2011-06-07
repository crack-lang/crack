// Copyright 2009 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include "FloatConst.h"

#include "builder/Builder.h"
#include "parser/ParseError.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

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
        newType == context.construct->float32Type)
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
