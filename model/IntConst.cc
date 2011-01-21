// Copyright 2009 Google Inc.

#include "IntConst.h"

#include "stdint.h"
#include "builder/Builder.h"
#include "parser/ParseError.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "FloatConst.h"

using namespace model;
using namespace parser;

IntConst::IntConst(TypeDef *type, int64_t val0) :
    Expr(type),
    val(),
    reqUnsigned(false) {
    val.sval = val0;
}

IntConst::IntConst(TypeDef *type, uint64_t val0) :
    Expr(type),
    val(),
    reqUnsigned(true) {
    val.uval = val0;
}

ResultExprPtr IntConst::emit(Context &context) { 
    return context.builder.emitIntConst(context, this);
}

ExprPtr IntConst::convert(Context &context, TypeDef *newType) {
    if (newType == this->type)
        return this;

    if (newType == context.construct->int64Type)
        ; // nothing we can do about this right now
    else if (newType == context.construct->uint64Type) {
        if (!reqUnsigned && val.sval < 0)
            return 0;
            // since we can't currently give these errors, at least record 
            // them in the comments.
            // context.error = "Negative constant can not be converted to "
            //                  "uint64"
    } else if (newType == context.construct->int32Type) {
        if (reqUnsigned ||
            val.sval > INT32_MAX ||
            val.sval < INT32_MIN)
            return 0;
            // context.error = "Constant out of range of int32"
    } else if ((newType == context.construct->float32Type) ||
               (newType == context.construct->float64Type)) {
        // XXX range check?
        return context.builder.createFloatConst(context,
                                                (double)val.sval,
                                                newType);
    } else if (newType == context.construct->uint32Type) {
        if ((!reqUnsigned && val.sval < 0) || val.uval > UINT32_MAX)
            return 0;
            // context.error = "Constant out of range of uint32"
    } else if (newType == context.construct->byteType) {
        if ((!reqUnsigned && val.sval < 0) || val.uval > UINT8_MAX) {
            return 0;
            // context.error = "Constant out of range of byte"
        }
    } else {
        // delegate all other conversions to the type
        return Expr::convert(context, newType);
    }

    return context.builder.createIntConst(context, val.sval, newType);
}

void IntConst::writeTo(std::ostream &out) const {
    out << val.sval;
}

bool IntConst::isAdaptive() const {
    return true;
}

// for selecting between int32 and int64
TypeDef *IntConst::selectType(Context &context, int64_t val) {
    // note, due to the way the parser processes large unsigned ints,
    // this shouldn't be called if val > INT64_MAX, i.e. when it requires
    // unsigned int 64
    if (val > INT32_MAX || val < INT32_MIN)
        return context.construct->int64Type.get();
    else
        return context.construct->intType.get();
}
