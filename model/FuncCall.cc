
#include "FuncCall.h"

#include "builder/Builder.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "TypeDef.h"

using namespace model;

void FuncCall::emit(Context &context) {
    FuncDefPtr func = FuncDefPtr::dcast(context.lookUp(name));
    context.builder.emitFuncCall(context, func, args);
}

    
