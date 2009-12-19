
#include "FuncCall.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "TypeDef.h"

using namespace model;

FuncCall::FuncCall(FuncDef *funcDef) :
    Expr(funcDef->type.get()),
    func(funcDef) {
}

void FuncCall::emit(Context &context) {
    context.builder.emitFuncCall(context, func.get(), receiver.get(), args);
}
