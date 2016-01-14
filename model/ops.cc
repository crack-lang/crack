// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ops.h"

#include "FloatConst.h"
#include "IntConst.h"

using namespace model;

model::ExprPtr NegOpCall::foldConstants() {
    ExprPtr val;
    if (receiver)
        val = receiver;
    else
        val = args[0];

    IntConstPtr v = IntConstPtr::rcast(val);
    if (v)
        return v->foldNeg();
    else
        return this;
}

ExprPtr BitNotOpCall::foldConstants() {
    ExprPtr val;
    if (receiver)
        val = receiver;
    else
        val = args[0];

    IntConstPtr v = IntConstPtr::rcast(val);
    if (v)
        return v->foldBitNot();
    else
        return this;
}

ExprPtr FNegOpCall::foldConstants() {
    FloatConstPtr fc = FloatConstPtr::rcast(receiver ? receiver : args[0]);
    if (fc)
        return fc->foldNeg();
    else
        return this;
}

#define REV_BINOPF(opCode, cls) \
    ExprPtr opCode##ROpCall::foldConstants() {                              \
        ExprPtr rval = receiver.get();                                      \
        ExprPtr lval = args[0].get();                                       \
        cls##ConstPtr ci = cls##ConstPtr::rcast(lval);                      \
        ExprPtr result;                                                     \
        if (ci)                                                             \
            result = ci->fold##opCode(rval.get());                          \
        return result ? result : this;                                      \
    }

// binary operation with folding
#define BINOPF(prefix, op, cls) \
    ExprPtr prefix##OpCall::foldConstants() {                               \
        ExprPtr lval = receiver ? receiver.get() : args[0].get();           \
        ExprPtr rval = receiver ? args[0].get() : args[1].get();            \
        cls##ConstPtr ci = cls##ConstPtr::rcast(lval);                      \
        ExprPtr result;                                                     \
        if (ci)                                                             \
            result = ci->fold##prefix(rval.get());                          \
        return result ? result : this;                                      \
    }

#define BINOPIF(prefix, op) BINOPF(prefix, op, Int)
#define REV_BINOPIF(prefix) REV_BINOPF(prefix, Int)
#define BINOPFF(prefix, op) BINOPF(prefix, op, Float)
#define REV_BINOPFF(prefix) REV_BINOPF(prefix, Float)

// Binary Ops
BINOPIF(Add, "+");
BINOPIF(Sub, "-");
BINOPIF(Mul, "*");
BINOPIF(SDiv, "/");
BINOPIF(UDiv, "/");
BINOPIF(SRem, "%");  // Note: C'99 defines '%' as the remainder, not modulo
BINOPIF(URem, "%");  // the sign is that of the dividend, not divisor.
BINOPIF(Or, "|");
BINOPIF(And, "&");
BINOPIF(Xor, "^");
BINOPIF(Shl, "<<");
BINOPIF(LShr, ">>");
BINOPIF(AShr, ">>");
REV_BINOPIF(Add)
REV_BINOPIF(Sub)
REV_BINOPIF(Mul)
REV_BINOPIF(SDiv)
REV_BINOPIF(UDiv)
REV_BINOPIF(SRem)
REV_BINOPIF(URem)
REV_BINOPIF(Or)
REV_BINOPIF(And)
REV_BINOPIF(Xor)
REV_BINOPIF(Shl)
REV_BINOPIF(LShr)
REV_BINOPIF(AShr)

// Floating point operations.
BINOPFF(FAdd, "+");
BINOPFF(FSub, "-");
BINOPFF(FMul, "*");
BINOPFF(FDiv, "/");
BINOPFF(FRem, "%");
REV_BINOPFF(FAdd)
REV_BINOPFF(FSub)
REV_BINOPFF(FMul)
REV_BINOPFF(FDiv)
REV_BINOPFF(FRem)
