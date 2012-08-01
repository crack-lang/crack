// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "ResultExpr.h"

#include "builder/Builder.h"
#include "CleanupFrame.h"
#include "Context.h"
#include "FuncCall.h"
#include "FuncDef.h"
#include "TypeDef.h"

using namespace model;

void ResultExpr::handleAssignment(Context &context) {
    // if the expression is productive, the assignment will just consume its 
    // reference.
    if (sourceExpr->isProductive())
        return;

    // the expression is non-productive: check for a bind function
    FuncCall::ExprVec args;
    FuncDefPtr bindFunc = context.lookUpNoArgs("oper bind", false, type.get());
    if (!bindFunc)
        return;
    
    // got a bind function: create a bind call and emit it.  (emit should 
    // return a ResultExpr for a void object, so we don't need to do anything 
    // special for it).
    FuncCallPtr bindCall = context.builder.createFuncCall(bindFunc.get());
    bindCall->receiver = this;
    bindCall->emit(context);
}

void ResultExpr::handleTransient(Context &context) {
    // we don't need to do anything if we're currently emitting cleanups or for
    // non-productive expressions.
    if (context.emittingCleanups || !sourceExpr->isProductive())
        return;
    
    // the expression is productive - clean it up
    forceCleanup(context);
}

void ResultExpr::forceCleanup(Context &context) {
    // check for a release function
    FuncDefPtr releaseFunc = context.lookUpNoArgs("oper release", false, 
                                                  type.get()
                                                  );
    if (!releaseFunc)
        return;
    
    // got a release function: create a release call and store it in the 
    // cleanups.
    FuncCallPtr releaseCall = 
        context.builder.createFuncCall(releaseFunc.get());
    releaseCall->receiver = this;
    context.cleanupFrame->addCleanup(releaseCall.get());
}

bool ResultExpr::isProductive() const {
    // result expressions are always non-productive, since they always 
    // reference the result of an existing expression.  This means that the 
    // ResultExpression returned from ResultExpression::emit() will treat a 
    // re-used result as a non-productive expression, which is what we want.
    return false;
}

void ResultExpr::writeTo(std::ostream &out) const {
    out << "result(" << *sourceExpr << ')';
}
