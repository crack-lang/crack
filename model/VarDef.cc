// Copyright 2009 Google Inc.

#include "VarDef.h"

#include "AssignExpr.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace std;
using namespace model;

VarDef::VarDef(TypeDef *type, const std::string &name) :
    type(type),
    name(name),
    owner(0),
    constant(false) {
}

VarDef::~VarDef() {}

ResultExprPtr VarDef::emitAssignment(Context &context, Expr *expr) {
    AssignExprPtr assign = new AssignExpr(0, this, expr);
    return impl->emitAssignment(context, assign.get());
}

bool VarDef::hasInstSlot() {
    return impl->hasInstSlot();
}

bool VarDef::isStatic() const {
    return false;
}

std::string VarDef::getFullName() const {
    if (!fullName.empty())
        return fullName;
    if (owner && !owner->getNamespaceName().empty())
        fullName = owner->getNamespaceName()+"."+name;
    else
        fullName = name;
    return fullName;
}

std::string VarDef::getDisplayName() const {
    assert(owner && "no owner defined when getting display name");
    std::string module = owner->getNamespaceName();
    if (!module.compare(0, 6, ".main.")) {
        // find the next namespace after the main script
        size_t pos = module.find('.', 6);
        if (pos != string::npos)
            return module.substr(pos + 1) + "." + name;
        else
            return name;
    } else if (module == ".builtin") {
        return name;
    } else {
        return getFullName();
    }
}

bool VarDef::isConstant() {
    return constant;
}

void VarDef::dump(ostream &out, const string &prefix) const {
    out << prefix << (type ? type->getFullName() : string("<null>")) << " " << name << endl;
}

void VarDef::dump() const {
    dump(std::cerr, "");
}
