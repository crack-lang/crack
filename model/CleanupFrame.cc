// Copyright 2009 Google Inc.

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
        varDef->type->context->lookUpNoArgs("oper release", false);
    
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
