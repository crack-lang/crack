
#include "VarDef.h"

#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"

using namespace model;

VarDef::VarDef(TypeDef *type, const std::string &name) :
    type(type),
    name(name),
    context(0) {
}

void VarDef::emitAssignment(Context &context, Expr *expr) {
    impl->emitAssignment(context, this, expr);
}

bool VarDef::hasInstSlot() {
    return true;
}
