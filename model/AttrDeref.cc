// Copyright 2019 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "AttrDeref.h"

#include <spug/StringFmt.h>
#include <builder/Builder.h>
#include "Context.h"
#include "FuncCall.h"

using namespace model;
using namespace std;

ExprPtr AttrDeref::createDeref(Context &context) const {
    FuncCallPtr fc = context.builder.createFuncCall(operGet.get(), false);
    fc->receiver = receiver;
    return fc;
}

// Note that the expression type doesn't get set until we either commit to
// doing an emit() or a makeCall().
AttrDeref::AttrDeref(Expr *receiver, OverloadDef *operSet, FuncDef *operGet,
                     TypeDef *noGetterType
                     ) :
    Deref(receiver),
    operSet(operSet),
    operGet(operGet) {

    if (operGet)
        type = operGet->returnType;
    else
        type = noGetterType;
}

ResultExprPtr AttrDeref::emit(Context &context) {
    if (!operGet)
        context.error("Attempting to evaluate attribute with setter but no "
                       "getter."
                      );

    ResultExprPtr result = createDeref(context)->emit(context);;
    return result;
}

void AttrDeref::writeTo(ostream &out) const {
    out << "AttrDeref(" << *receiver << ", ";
    if (operSet)
        out << *operSet;
    else
        out << "null";

    if (operGet)
        out << *operGet;
    else
        out << "null";
}

ExprPtr AttrDeref::convert(Context &context, TypeDef *type) {
    if (!operGet)
        context.error("Attempt to evaluate attribute with no getter.");
    return Deref::convert(context, type);
}

ExprPtr AttrDeref::makeAssignment(Context &context, Expr *val) {
    if (!operSet)
        context.error(SPUG_FSTR("No setter specified for attribute with '" <<
                                operGet->name << "'"));

    FuncCall::ExprVec args;
    args.push_back(val);
    FuncDefPtr setter = operSet->getMatch(context, args, false);
    if (!setter)
        context.error(SPUG_FSTR("No setter sepecified for attribute with "));

    FuncCallPtr fc = context.builder.createFuncCall(setter.get(), false);
    fc->args = args;
    fc->receiver = receiver;

    // Correct the return type to be that of the selected setter function.
    type = setter->returnType;

    return fc;
}
