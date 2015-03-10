// Copyright 2014 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Deref.h"

#include <sstream>

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

    // Make sure the def is not a method. (We may want to handle this case,
    // eventually, either by returning a Python-style bound method or more
    // likely by returning the function address, dereferencing virtual
    // functions if necessary)
    if (OverloadDefPtr::rcast(def))
        context.error("Bound methods can not be evaluated.");

    return context.builder.createFieldRef(receiver.get(),
                                          def.get()
                                          )->emit(context);
}

void Deref::writeTo(ostream &out) const {
    out << "Deref(" << *receiver << ", " << *def << ")";
}

ExprPtr Deref::makeCall(Context &context, FuncCall::ExprVec &args) const {
    OverloadDefPtr ovldDef = OverloadDefPtr::rcast(def);
    if (!ovldDef) {
        VarRefPtr ref = context.createFieldRef(receiver.get(), def.get());
        return ref->makeCall(context, args);
    }

    FuncDefPtr funcDef = ovldDef->getMatch(context, args, false);
    if (!funcDef) {
        ostringstream msg;
        msg << "No method exists matching " << ovldDef->name << "(" <<
            args << ")";
        context.maybeExplainOverload(msg, ovldDef->name, ovldDef->getOwner());
        context.error(msg.str());
    }
    FuncCallPtr funcCall = context.builder.createFuncCall(funcDef.get(),
                                                          squashVirtual
                                                          );
    funcCall->args = args;

    // Even though we're in a deref, certain kinds of functions (i.e. "cast")
    // aren't methods.
    if (funcDef->flags & FuncDef::method)
        funcCall->receiver = receiver;

    return funcCall;
}

ExprPtr Deref::makeAssignment(Context &context, Expr *val) {
    return AssignExpr::create(context, receiver.get(), def.get(), val);
}
