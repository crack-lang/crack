// Copyright 2009-2012 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "VarDef.h"

#include "AssignExpr.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "ResultExpr.h"
#include "Serializer.h"
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

ModuleDef *VarDef::getModule() const {
    return owner->getModule().get();
}

void VarDef::addDependenciesTo(set<string> &deps) const {
    getModule()->computeDependencies(deps);
}

namespace {
    enum DefTypes {
        variableId = 1,
        typeId = 2,
        genericId = 3,
        functionId = 4,
        aliasId = 5
    };
}

void VarDef::serializeExtern(Serializer &serializer) const {
    if (serializer.writeObject(this)) {
        serializer.write(getModule()->getFullName());
        serializer.write(name);
    }
}

void VarDef::serializeAlias(Serializer &serializer, const string &alias) const {
    serializer.write(aliasId);
    serializer.write(alias);
    serializeExtern(serializer);
}

void VarDef::serialize(Serializer &serializer) const {
    serializer.write(variableId);
    serializer.write(name);
    type->serialize(serializer);
}
