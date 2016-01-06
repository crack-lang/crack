// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ops.h"

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

NegOpDef::NegOpDef(TypeDef *resultType, const std::string &name,
                   bool isMethod
                   ) :
        OpDef(resultType,
              FuncDef::builtin |
               (isMethod ? FuncDef::method : FuncDef::noFlags),
              name,
              isMethod ? 0 : 1
              ) {
    if (!isMethod)
        args[0] = new ArgDef(resultType, "operand");
}
