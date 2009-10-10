
#include "VarDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"

using namespace model;

VarDef::VarDef(const TypeDefPtr &type, const std::string &name) :
    Def(name),
    type(type) {
}
