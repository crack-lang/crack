
#include "VarRef.h"

#include "builder/Builder.h"
#include "Context.h"
#include "VarDef.h"

using namespace model;

VarRef::VarRef(const VarDefPtr &def) : def(def) {}

void VarRef::emit(Context &context) {
    context.builder.emitVarRef(context, *this);
}

