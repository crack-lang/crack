
#include "AllocExpr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

void AllocExpr::emit(Context &context) {
    context.builder.emitAlloc(context, type.get());
}

void AllocExpr::writeTo(ostream &out) const {
    out << "alloc(" << type->name << ")";
}
