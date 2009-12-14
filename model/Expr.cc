
#include "Expr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;

void Expr::emitCond(Context &context) {
    context.builder.emitTest(context, this);
}
