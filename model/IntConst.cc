// Copyright 2009-2011 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "IntConst.h"

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
    reqUnsigned(false),
    uneg(false) {
    val.sval = val0;
}

IntConst::IntConst(TypeDef *type, uint64_t val0) :
    Expr(type),
    val(),
    reqUnsigned(val0 > INT64_MAX),
    uneg(false) {
    val.uval = val0;
}

ResultExprPtr IntConst::emit(Context &context) { 
    return context.builder.emitIntConst(context, this);
}

ExprPtr IntConst::convert(Context &context, TypeDef *newType) {
    if (newType == this->type)
        return this;

    if (newType == context.construct->int64Type ||
        newType == context.construct->intType &&
        context.construct->intSize == 64 ||
        newType == context.construct->intzType &&
        context.construct->intzSize == 64
        ) {
        if (reqUnsigned)
            return 0;
    } else if (newType == context.construct->uint64Type ||
               newType == context.construct->uintType &&
               context.construct->intSize == 64
               ) {
        if (!(reqUnsigned || uneg) && val.sval < 0)
            return 0;
            // since we can't currently give these errors, at least record 
            // them in the comments.
            // context.error = "Negative constant can not be converted to "
            //                  "uint64"
    } else if (newType == context.construct->int32Type ||
               newType == context.construct->intType ||
               newType == context.construct->intzType
               ) {
        if (reqUnsigned ||
            val.sval > static_cast<int64_t>(INT32_MAX) ||
            val.sval < static_cast<int64_t>(INT32_MIN))
            return 0;
            // context.error = "Constant out of range of int32"
    } else if (newType == context.construct->int16Type) {
        if (reqUnsigned ||
            val.sval > static_cast<int64_t>(INT16_MAX) ||
            val.sval < static_cast<int64_t>(INT16_MIN))
            return 0;
            // context.error = "Constant out of range of int16"
    } else if (newType == context.construct->float32Type ||
               newType == context.construct->float64Type ||
               newType == context.construct->floatType
               ) {
        // XXX range check?
        return context.builder.createFloatConst(context,
                                                (double)val.sval,
                                                newType);
    } else if (newType == context.construct->uint32Type ||
               newType == context.construct->uintType ||
               newType == context.construct->uintzType
               ) {
        if ((uneg && (val.sval < -static_cast<uint64_t>(UINT32_MAX + 1) ||
                      val.sval >= 0
                      )
             ) ||
            (!uneg && val.uval > static_cast<uint64_t>(UINT32_MAX))
            )
            return 0;
            // context.error = "Constant out of range of uint32"
    } else if (newType == context.construct->uint16Type) {
        if ((uneg && (val.sval < -static_cast<uint64_t>(UINT16_MAX + 1) ||
                      val.sval >= 0
                      )
             ) ||
            (!uneg && val.uval > static_cast<uint64_t>(UINT16_MAX))
            )
            return 0;
            // context.error = "Constant out of range of uint16"
    } else if (newType == context.construct->byteType) {
        // to convert to a byte either
        // 1) we're uneg and in -255 .. -1
        // 2) we're not uneg and in 0 .. 255
        if ((uneg && (val.sval < -(UINT8_MAX + 1) || val.sval >= 0)) ||
            (!uneg && val.uval > UINT8_MAX)
            ) {
            return 0;
            // context.error = "Constant out of range of byte"
        }
    // check for the PDNTs, which are always safe to convert to.
    } else if (!(newType == context.construct->intType ||
                 newType == context.construct->uintType ||
                 newType == context.construct->intzType ||
                 newType == context.construct->uintzType
                 )
               ) {
        // delegate all other conversions to the type
        return Expr::convert(context, newType);
    }

    if (reqUnsigned)
        return context.builder.createUIntConst(context, val.uval, newType);
    else
        return context.builder.createIntConst(context, val.sval, newType);
}

void IntConst::writeTo(std::ostream &out) const {
    out << val.sval;
}

bool IntConst::isAdaptive() const {
    return true;
}

IntConstPtr IntConst::create(int64_t v) { return new IntConst(type.get(), v); }
IntConstPtr IntConst::create(uint64_t v) { return new IntConst(type.get(), v); }

// for selecting the default type of a constant
TypeDef *IntConst::selectType(Context &context, int64_t val) {
    // note, due to the way the parser processes large unsigned ints,
    // this shouldn't be called if val > INT64_MAX, i.e. when it requires
    // unsigned int 64
    if (val > INT32_MAX || val < INT32_MIN)
        return context.construct->int64Type.get();
    else return context.construct->intType.get();
}

#define HIBIT(c) (c)->val.sval & 0x8000000000000000 ? true : false

// folding for bitwise operations (use the operation to determine uneg)
#define FOLDB(name, sym, signed) \
    ExprPtr IntConst::fold##name(Expr *other) {                             \
        IntConstPtr o = IntConstPtr::cast(other);                           \
        if (o) {                                                            \
            bool newUneg = uneg sym o->uneg;                                \
            o = create(val.signed##val sym o->val.signed##val);             \
            o->uneg = newUneg;                                              \
            o->reqUnsigned = HIBIT(o);                                      \
            return o;                                                       \
        } else {                                                            \
            return 0;                                                       \
        }                                                                   \
    }

// folding for arithmetic operations (result is never uneg and never "req 
// unsigned" - just to keep things simple)
#define FOLDA(name, sym, signed) \
    ExprPtr IntConst::fold##name(Expr *other) {                             \
        IntConstPtr o = IntConstPtr::cast(other);                           \
        if (o) {                                                            \
            o = create(val.signed##val sym o->val.signed##val);             \
            return o;                                                       \
        } else {                                                            \
            return 0;                                                       \
        }                                                                   \
    }

// folding for arithmetic shift operations (result uneg is left-operand uneg)
#define FOLDL(name, sym, signed) \
    ExprPtr IntConst::fold##name(Expr *other) {                             \
        IntConstPtr o = IntConstPtr::cast(other);                           \
        if (o) {                                                            \
            o = create(val.signed##val sym o->val.signed##val);             \
            o->uneg = uneg;                                                 \
            o->reqUnsigned = HIBIT(o) != uneg;                              \
            return o;                                                       \
        } else {                                                            \
            return 0;                                                       \
        }                                                                   \
    }

FOLDA(Add, +, s)
FOLDA(Sub, -, s)
FOLDA(Mul, *, s)
FOLDA(SDiv, /, s)
FOLDA(UDiv, /, u)
FOLDA(SRem, %, s)
FOLDA(URem, %, u)
FOLDB(Or, |, u)
FOLDB(And, &, u)
FOLDB(Xor, ^, u)

ExprPtr IntConst::foldNeg() {
    IntConstPtr result = create(-val.sval);
    result->uneg = false;
    return result;
}

ExprPtr IntConst::foldBitNot() {
    IntConstPtr result = create(~val.sval);
    result->uneg = !uneg;
    result->reqUnsigned = reqUnsigned;
    return result;
}

// the shift operators are unique WRT their treatement of uneg and reqUnsigned

ExprPtr IntConst::foldAShr(Expr *other) {
    IntConstPtr o = IntConstPtr::cast(other);
    if (o) {
        // convert the shift to a logical shift if the constant must be an 
        // unsigned
        o = reqUnsigned ? create(val.uval >> o->val.sval) :
                          create(val.sval >> o->val.sval);
        o->uneg = uneg;

        // shift right is easy: unsigned is never required.
        o->reqUnsigned = false;
        return o;
    } else {
        return 0;
    }
}

ExprPtr IntConst::foldLShr(Expr *other) {
    IntConstPtr o = IntConstPtr::cast(other);
    if (o) {
        o = create(val.uval >> o->val.sval);
        o->uneg = uneg;

        // shift right is easy: unsigned is never required.
        o->reqUnsigned = false;
        return o;
    } else {
        return 0;
    }
}

#define HISET 0x8000000000000000LL

ExprPtr IntConst::foldShl(Expr *other) {
    IntConstPtr o = IntConstPtr::cast(other);
    if (o) {
        o = create(val.sval << o->val.sval);
        o->uneg = uneg;
        
        // a left-shift requires negative if any of the bits being shifted 
        // away are not of the same polarity of the sign.
        bool negative = uneg || (reqUnsigned && HIBIT(this));
        o->reqUnsigned = negative ? (HISET >> (o->val.sval + 1)) & ~val.uval :
                                    (HISET >> (o->val.sval + 1)) & val.uval;
            
        return o;
    } else {
        return 0;
    }
}


