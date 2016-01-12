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
