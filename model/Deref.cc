// Copyright 2014 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Deref.h"

#include "spug/check.h"
#include "builder/Builder.h"
#include "AssignExpr.h"
#include "Context.h"
#include "FuncCall.h"
#include "OverloadDef.h"
#include "VarDef.h"
#include "VarRef.h"

using namespace model;
using namespace std;

Deref::Deref(Expr *receiver, VarDef *def) :
    Expr(def->type.get()),
    receiver(receiver),
    def(def),
    squashVirtual(false) {
}

ResultExprPtr Deref::emit(Context &context) {
    // This is being treated as a FieldRef.
    return context.builder.createFieldRef(receiver.get(),
                                          def.get()
                                          )->emit(context);
}

void Deref::writeTo(ostream &out) const {
    out << "Deref(" << *receiver << ", " << *def << ")";
}

ExprPtr Deref::makeCall(Context &context, FuncCall::ExprVec &args) const {
    OverloadDefPtr ovldDef = def;
    if (!ovldDef)
        // XXX also need to handle fields with callable objects and classes
        // with constructors.  Also need a better damned error message.
        context.error("Can not call this field.");

    FuncDefPtr funcDef = ovldDef->getMatch(context, args, false);
    FuncCallPtr funcCall = context.builder.createFuncCall(funcDef.get(),
                                                          squashVirtual
                                                          );
    funcCall->args = args;
    funcCall->receiver = receiver;
    return funcCall;
}

ExprPtr Deref::makeAssignment(Context &context, Expr *val) {
    return AssignExpr::create(context, receiver.get(), def.get(), val);
}
