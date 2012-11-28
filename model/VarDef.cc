// Copyright 2009-2012 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "VarDef.h"

#include "builder/Builder.h"
#include "spug/check.h"
#include "AssignExpr.h"
#include "Deserializer.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "Expr.h"
#include "ModuleDefMap.h"
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

void VarDef::addDependenciesTo(const ModuleDef *mod,
                               ModuleDefMap &deps
                               ) const {

    ModuleDefPtr depMod = getModule();
    if (depMod != mod &&
        deps.find(depMod->getNamespaceName()) == deps.end()
        )
        deps[depMod->getNamespaceName()] = depMod;

    // add the dependencies of the type
    if (type.get() != this)
        type->addDependenciesTo(mod, deps);
}

void VarDef::serializeExtern(Serializer &serializer) const {
    if (serializer.writeObject(this, "ext")) {
        serializer.write(getModule()->getFullName(), "module");
        serializer.write(name, "name");
    }
}

void VarDef::serializeAlias(Serializer &serializer, const string &alias) const {
    serializer.write(Serializer::aliasId, "kind");
    serializer.write(alias, "alias");
    serializeExtern(serializer);
}

namespace {
    struct AliasReader : public Deserializer::ObjectReader {
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            string moduleName = deser.readString(Serializer::modNameSize,
                                                 "module"
                                                 );
            string name = deser.readString(Serializer::varNameSize, "name");

            ModuleDefPtr mod = deser.context->construct->getModule(moduleName);
            SPUG_CHECK(mod,
                       "Deserializing " << moduleName << "." << name <<
                        ": module could not be resolved."
                       );
            VarDefPtr var = mod->lookUp(name);
            SPUG_CHECK(mod,
                       "Deserializing " << moduleName << "." << name <<
                        ": name not defined in module."
                       );
            return var;
        }
    };
}

VarDefPtr VarDef::deserializeAlias(Deserializer &serializer) {
    return VarDefPtr::rcast(serializer.readObject(AliasReader(),
                                                  "ext"
                                                  ).object
                            );
}

void VarDef::serialize(Serializer &serializer, bool writeKind) const {
    if (writeKind)
        serializer.write(Serializer::variableId, "kind");
    serializer.write(name, "name");
    type->serialize(serializer, false);
}

VarDefPtr VarDef::deserialize(Deserializer &deser) {
    string name = deser.readString(16, "name");
    TypeDefPtr type = TypeDef::deserialize(deser);
    return deser.context->builder.materializeVar(*deser.context, name,
                                                 type.get()
                                                 );
}