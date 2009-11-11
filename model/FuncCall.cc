
#include "FuncCall.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "TypeDef.h"

using namespace model;

FuncCall::FuncCall(const FuncDefPtr &funcDef) :
    Expr(funcDef->type),
    func(funcDef) {
}

void FuncCall::emit(Context &context) {
    context.builder.emitFuncCall(context, func, receiver, args);
}

    
