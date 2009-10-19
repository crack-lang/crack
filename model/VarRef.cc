
#include "VarRef.h"

#include "builder/Builder.h"
#include "BuilderVarDefData.h"
#include "Context.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;

VarRef::VarRef(const VarDefPtr &def) :
    Expr(def->type),
    def(def) {
}

void VarRef::emit(Context &context) {
    context.builder.emitVarRef(context, *this);
}

