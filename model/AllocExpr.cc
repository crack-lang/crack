
#include "AllocExpr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;

void AllocExpr::emit(Context &context) {
    context.builder.emitAlloc(context, type.get());
}
