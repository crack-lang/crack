
#include "NullConst.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

void NullConst::emit(Context &context) {
    context.builder.emitNull(context, *type);
}

ExprPtr NullConst::convert(Context &context, TypeDef *newType) {
    return new NullConst(newType);
}

