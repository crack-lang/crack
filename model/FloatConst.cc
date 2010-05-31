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
    /*
    if (newType == context.globalData->int64Type)
        ; // nothing we can do about this right now
    else if (newType == context.globalData->uint64Type) {
        if (val < 0)
            throw ParseError("Negative constant can not be converted to "
                              "uint64"
                             );
    } else if (newType == context.globalData->int32Type) {
        if (val > (1L << 31 - 1) || val < -(1L << 31 - 1))
            throw ParseError("Constant out of range of int32");
    } else if (newType == context.globalData->uint32Type) {
        if (val < 0 || val > (1LL << 32 - 1)) {
            throw ParseError("Constant out of range of uint32");
        }
    } else {
        // delegate all other conversions to the type
        return Expr::convert(context, newType);
    }
    */

    // XXX

    return Expr::convert(context, newType);

    //return context.builder.createFloatConst(context, val, newType);
}

void FloatConst::writeTo(std::ostream &out) const {
    out << val;
}
