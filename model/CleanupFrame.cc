// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "CleanupFrame.h"

#include "builder/Builder.h"
#include "Context.h"
#include "FuncCall.h"
#include "FuncDef.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarRef.h"

using namespace model;

void CleanupFrame::addCleanup(VarDef *varDef, Expr *aggregate) {
    // we only need to do cleanups for types with a "release" function
    FuncDefPtr releaseFunc =
        context->lookUpNoArgs("oper release", false, varDef->type.get());
    
    if (releaseFunc) {
        VarRefPtr varRef;
        if (aggregate)
            varRef = context->builder.createFieldRef(aggregate, varDef);
        else
            varRef = context->builder.createVarRef(varDef);
        FuncCallPtr funcCall = 
            context->builder.createFuncCall(releaseFunc.get());
        funcCall->receiver = varRef;
        addCleanup(funcCall.get());
    }
}
