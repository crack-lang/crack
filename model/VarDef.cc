
#include "VarDef.h"

#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"

using namespace model;

VarDef::VarDef(const TypeDefPtr &type, const std::string &name) :
    type(type),
    name(name),
    context(0) {
}

void VarDef::emitAssignment(Context &context, const ExprPtr &expr) {
    impl->emitAssignment(context, expr);
}

