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
#include "ModuleStub.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "Serializer.h"
#include "TypeDef.h"

using namespace std;
using namespace model;

VarDef::VarDef(TypeDef *type, const std::string &name) :
    type(type),
    name(name),
    owner(0),
    constant(false),
    stubFree(false) {
}

VarDef::~VarDef() {}

ResultExprPtr VarDef::emitAssignment(Context &context, Expr *expr) {
    AssignExprPtr assign = new AssignExpr(0, this, expr);
    return impl->emitAssignment(context, assign.get());
}

bool VarDef::hasInstSlot() {
    return impl->hasInstSlot();
}

int VarDef::getInstSlot() const {
    return impl->getInstSlot();
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

bool VarDef::isHidden() const {
    return owner->isHiddenScope();
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

namespace {
    bool isExported(const Namespace *ns, const string &name) {
        const ModuleDef *modDef = dynamic_cast<const ModuleDef *>(ns);
        return modDef && (modDef->exports.find(name) != modDef->exports.end());
    }
}

bool VarDef::isSerializable(const Namespace *ns, const string &name) const {
    return name[0] != ':' && (owner == ns || isExported(ns, name));
}

void VarDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {

    ModuleDefPtr depMod = getModule();
    mod->addDependency(depMod.get());

    // add the dependencies of the type
    if (type.get() != this)
        type->addDependenciesTo(mod, added);
}

void VarDef::serializeExternCommon(Serializer &serializer,
                                   const TypeDef::TypeVec *typeParams
                                   ) const {
    serializer.write(getModule()->getFullName(), "module");
    serializer.write(name, "name");

    if (typeParams) {
        // write the list we've accumulated.
        serializer.write(typeParams->size(), "#typeParams");
        for (TypeDef::TypeVec::const_iterator iter = typeParams->begin();
            iter != typeParams->end();
            ++iter
            )
            (*iter)->serialize(serializer, false, 0);
    } else {
        serializer.write(0, "#typeParams");
    }
}

void VarDef::serializeExternRef(Serializer &serializer,
                                const TypeDef::TypeVec *localDeps
                                ) const {
    if (serializer.writeObject(this, "ext"))
        serializeExternCommon(serializer, localDeps);
}

void VarDef::serializeExtern(Serializer &serializer) const {
    serializeExternCommon(serializer, 0);
}

void VarDef::serializeAlias(Serializer &serializer, const string &alias) const {
    serializer.write(Serializer::aliasId, "kind");
    serializer.write(alias, "alias");
    serializeExternRef(serializer, 0);
}

namespace {

    enum SymbolKind {
        K_TYPE,
        K_OVLD,
        K_VAR
    };

    TypeDef::TypeVecObjPtr parseTypeParameters(Context &context,
                                               const string &typeName,
                                               int parmStart
                                               );

    VarDefPtr resolveName(Context &context, const string &moduleName,
                           const string &symbolName,
                           SymbolKind kind
                           ) {
        ModuleDefPtr module = context.construct->getModule(moduleName);
        SPUG_CHECK(module,
                   "Unable to find module " << moduleName <<
                    " which contains referenced symbol " << symbolName
                   );

        // if this is an unfinished module, it should be a placeholder.  Use
        // it to create a stub for the symbol.
        if (!module->finished) {
            ModuleStubPtr stub = ModuleStubPtr::rcast(module);
            SPUG_CHECK(stub,
                       "Referenced module " << module->getFullName() <<
                        " is not finished, but isn't a stub."
                       )
            stub->dependents.insert(
                ModuleDefPtr::arcast(context.getModuleContext()->ns)
            );
            switch (kind) {
                case K_TYPE:
                    return stub->getTypeStub(symbolName);
                case K_OVLD:
                    return stub->getOverloadStub(symbolName);
                case K_VAR:
                    return stub->getVarStub(symbolName);
                default:
                    SPUG_CHECK(false, "Unknoan symbol kind: " << kind);
            }
        }

        VarDefPtr var = module->lookUp(symbolName);
        SPUG_CHECK(var,
                   "Unable to find symbol " << moduleName << "." <<
                    symbolName
                   );
        return var;
    }

    VarDefPtr deserializeAliasBody(Deserializer &deser, SymbolKind kind) {
        string moduleName = deser.readString(Serializer::modNameSize,
                                            "module"
                                            );
        string name = deser.readString(Serializer::varNameSize, "name");

        // deserialize type parameters.
        int paramCount = deser.readUInt("#typeParams");
        TypeDef::TypeVecObjPtr paramTypes = new TypeDef::TypeVecObj();
        paramTypes->reserve(paramCount);
        for (int i = 0; i < paramCount; ++i)
            paramTypes->push_back(TypeDef::deserialize(deser));

        VarDefPtr varDef = resolveName(*deser.context, moduleName, name, kind);
        if (paramCount) {
            TypeDefPtr typeDef = varDef;
            return typeDef->getSpecialization(*deser.context, paramTypes.get());
        } else {
            return varDef;
        }
    }


} // anon namespace

TypeDefPtr VarDef::deserializeTypeAliasBody(Deserializer &deser) {
    return deserializeAliasBody(deser, K_TYPE);
}

OverloadDefPtr VarDef::deserializeOverloadAliasBody(Deserializer &deser) {
    return deserializeAliasBody(deser, K_OVLD);
}

VarDefPtr VarDef::deserializeVarAliasBody(Deserializer &deser) {
    return deserializeAliasBody(deser, K_VAR);
}

namespace {
    struct AliasReader : public Deserializer::ObjectReader {
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            return VarDef::deserializeVarAliasBody(deser);
        }
    };
}

VarDefPtr VarDef::deserializeAlias(Deserializer &serializer) {
    return VarDefPtr::rcast(serializer.readObject(AliasReader(),
                                                  "ext"
                                                  ).object
                            );
}

void VarDef::serialize(Serializer &serializer, bool writeKind,
                       const Namespace *ns
                       ) const {
    if (writeKind)
        serializer.write(Serializer::variableId, "kind");
    serializer.write(name, "name");
    serializer.write(getInstSlot() + 1, "instSlot");
    type->serialize(serializer, false, ns);
}

VarDefPtr VarDef::deserialize(Deserializer &deser) {
    string name = deser.readString(16, "name");
    int instSlot = static_cast<int>(deser.readUInt("instSlot")) - 1;
    TypeDefPtr type = TypeDef::deserialize(deser);
    return deser.context->builder.materializeVar(*deser.context, name,
                                                 type.get(),
                                                 instSlot
                                                 );
}

VarDefPtr VarDef::replaceAllStubs(Context &context) {
    if (stubFree)
        return this;
    stubFree = true;
    VarDefPtr replacement = replaceStub(context);
    if (replacement)
        return replacement;

    if (type)
        type = type->replaceAllStubs(context);
    return this;
}
