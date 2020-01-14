// Copyright 2020 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "SafeNavExpr.h"

#include "builder/Builder.h"

#include "FuncCall.h"
#include "NullConst.h"
#include "TypeDef.h"

using namespace model;

void SafeNavExpr::writeTo(std::ostream &out) const {
    cond->writeTo(out);
    out << "?.";
    falseVal->writeTo(out);
}

ExprPtr SafeNavExpr::makeCall(Context &context,
                              std::vector<ExprPtr> &args
                              ) const {

    ExprPtr call = falseVal->makeCall(context, args);
    if (call->type == context.construct->voidType) {

        // If the result is void, we can't create a null constant for it, but
        // we can omit the "false" condition in the ternary expression,
        // meaning that we can use the non-null case as the true expression.
        //
        // Create a function call to invert the condition.
        FuncCall::ExprVec args(1);
        args[0] = cond;
        FuncDefPtr func = context.lookUp("oper !", args);
        assert(func);
        FuncCallPtr newCond = context.builder.createFuncCall(func.get());
        newCond->args = args;
        return new TernaryExpr(newCond.get(), call.get(), 0,
                               call->type.get());
    } else {
         return new TernaryExpr(cond.get(), new NullConst(call->type.get()),
                                call.get(),
                                call->type.get());
    }
}
