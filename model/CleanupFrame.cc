
#include "CleanupFrame.h"

#include "builder/Builder.h"
#include "Context.h"
#include "FuncCall.h"
#include "FuncDef.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarRef.h"

using namespace model;

void CleanupFrame::addCleanup(VarDef *varDef) {
    // we only need to do cleanups for types with a "release" function
    FuncCall::ExprVec args;
    FuncDefPtr releaseFunc =
        varDef->type->context->lookUp(*context, "release", args);
    
    if (releaseFunc) {
        VarRefPtr varRef = context->builder.createVarRef(varDef);
        FuncCallPtr funcCall = 
            context->builder.createFuncCall(releaseFunc.get());
        funcCall->receiver = varRef;
        addCleanup(funcCall.get());
    }
}
