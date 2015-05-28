// Copyright 2009-2012 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "VarDef.h"

#include <sstream>

#include "spug/StringFmt.h"

#include "builder/Builder.h"
#include "spug/check.h"
#include "AssignExpr.h"
#include "Deserializer.h"
#include "GlobalNamespace.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "CompositeNamespace.h"
#include "Expr.h"
#include "ModuleDefMap.h"
#include "NestedDeserializer.h"
#include "OverloadDef.h"
#include "ProtoBuf.h"
#include "ResultExpr.h"
#include "Serializer.h"
#include "StubDef.h"
#include "TypeDef.h"

using namespace std;
using namespace model;

VarDef::VarDef(TypeDef *type, const std::string &name) :
    type(type),
    name(name),
    owner(0),
    constant(false),
    exposed(false) {
}

VarDef::~VarDef() {}

ResultExprPtr VarDef::emitAssignment(Context &context, Expr *expr) {
    AssignExprPtr assign = new AssignExpr(0, this, expr);
    return impl->emitAssignment(context, assign.get());
}

FuncDefPtr VarDef::getFuncDef(Context &context,
                              std::vector<ExprPtr> &args,
                              bool allowOverrides
                              ) const {
    OverloadDefPtr ovld = context.lookUp("oper call", type.get());
    if (!ovld)
        context.error(SPUG_FSTR("Instance of " << type->getDisplayName() <<
                                 " does not have an 'oper call' method "
                                 " and can not be called."
                                )
                      );
    FuncDefPtr func = ovld->getMatch(context, args, true);
    if (!func) {
        ostringstream msg;
        msg << "Instance of " << type->getDisplayName() <<
            " cannot be called with arguments (" << args << ")" << endl;
        context.maybeExplainOverload(msg, ovld->name, type.get());
        context.error(msg.str());
    }
    return func;
}


bool VarDef::hasInstSlot() const {
    return impl && impl->hasInstSlot();
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

bool VarDef::isImportableFrom(ModuleDef *module, const string &impName) const {
    return owner->getModule().get() == module ||
           module->exports.find(impName) != module->exports.end();
}

bool VarDef::isImportable(const Namespace *ns, const string &name) const {
    // A symbol is importable if:
    // 1) it is either
    //  a) defined in the module we are importing it from or
    //  b) explicitly exported from the module we are importing it from
    //    (the "second order import" rules) or
    //  c) defined in a base class of the current class _and_
    // 2) It is either non-private (no leading underscore) or type-scoped and
    //    not class-private.
    TypeDef *asType = TypeDefPtr::cast(owner);
    const TypeDef *nsAsType = dynamic_cast<const TypeDef *>(ns);
    return (owner->getRealModule() ==
            const_cast<Namespace *>(ns)->getRealModule() ||
            asType && nsAsType && nsAsType->isDerivedFrom(asType) ||
            isExported(ns, name)
            ) &&
           (name[0] != '_' ||
            (asType && name.substr(0, 2) != "__")
            );
}

bool VarDef::isUsableFrom(const Context &context) const {
    // This is always true if it's not an instance variable.
    if (getInstSlot() == -1)
        return true;

    return context.hasInstanceOf(TypeDefPtr::cast(owner));
}

bool VarDef::needsReceiver() const {
    return impl && impl->isInstVar();
}

bool VarDef::isSerializable() const {
    return name[0] != ':';
}

void VarDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {

    ModuleDefPtr depMod = getModule();
    mod->addDependency(depMod.get());

    // add the dependencies of the type
    if (type && type.get() != this)
        type->addDependenciesTo(mod, added);
}

void VarDef::serializeExternCommon(Serializer &serializer,
                                   const TypeDef::TypeVec *localDeps
                                   ) const {
    ModuleDefPtr module = getModule();
    serializer.write(module->getFullName(), "module");

    // calculate the module relative name.
    list<string> moduleRelativeName;
    moduleRelativeName.push_front(name);
    NamespacePtr cur = getOwner();
    while (cur != module.get()) {
        // Convert to a VarDef, push the name segment.
        VarDef *def = cur->asVarDef();
        SPUG_CHECK(def,
                   "namespace " << cur->getNamespaceName() <<
                    " is not a VarDef."
                   );
        moduleRelativeName.push_front(def->name);

        cur = cur->getNamespaceOwner();
    }

    // is this a shared library function?
    const FuncDef *funcDef = dynamic_cast<const FuncDef *>(this);
    if (funcDef && !(funcDef->flags & FuncDef::shlib))
        // if it's not a shared library function, clear it so we don't a field
        // for it.
        funcDef = 0;

    ostringstream tmp;
    Serializer sub(serializer, tmp);

    // For primitive generic specializations, the "name" attribute currently
    // contains the parameters, so we trim them.
    const TypeDef *asType = dynamic_cast<const TypeDef *>(this);
    if (asType && asType->primitiveGenericSpec) {
        sub.write(CRACK_PB_KEY(3, string), "name.header");
        sub.write(name.substr(0, name.find('[')), "name");
        localDeps = &asType->genericParms;
    } else {
        // write the full name.
        for (list<string>::iterator i = moduleRelativeName.begin();
            i != moduleRelativeName.end();
            ++i
            ) {
            sub.write(CRACK_PB_KEY(3, string), "name.header");
            sub.write(*i, "name");
        }
    }

    if (funcDef) {
        ostringstream funcDefData;
        Serializer funcDefSerializer(funcDefData);
        funcDef->serializeCommon(funcDefSerializer);
        sub.write(CRACK_PB_KEY(1, string), "shlibFuncDef.header");
        sub.write(funcDefData.str(), "shlibFuncDef.body");
    }

    // write the local deps list.
    if (localDeps) {
        for (TypeDef::TypeVec::const_iterator iter = localDeps->begin();
            iter != localDeps->end();
            ++iter
            ) {
            sub.write(CRACK_PB_KEY(2, ref), "localDeps.header");
            (*iter)->serialize(sub, false, 0);
        }
    }

    serializer.write(tmp.str(), "optional");
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
    serializer.write(0, "optional");
}

namespace {

    enum SymbolKind {
        K_TYPE,
        K_OVLD,
        K_VAR
    };

    template<typename T>
    struct IterPair {
        typedef T iterator;
        T beginVal, endVal;
        T begin() const { return beginVal; }
        T end() const { return endVal; }
        IterPair(const T &beginVal, const T &endVal) :
            beginVal(beginVal),
            endVal(endVal) {
        }

        template <typename Container>
        IterPair(Container &container) :
            beginVal(container.begin()),
            endVal(container.end()) {
        }
    };

    template <typename T>
    ostream &operator <<(ostream &out, const IterPair<T> &val) {
        T i = val.begin();
        out << *i;
        ++i;
        for (; i != val.end(); ++i)
            out << "." << *i;
        return out;
    }

    template <typename T>
    IterPair<T> makeIterPair(const T &begin, const T &end) {
        return IterPair<T>(begin, end);
    }

    template <typename T>
    IterPair<typename T::iterator> makeIterPair(T &container) {
        return IterPair<typename T::iterator>(container.begin(),
                                              container.end()
                                              );
    }

    VarDefPtr resolvePath(Context &context, const string &moduleName,
                          list<string>::iterator curName,
                          list<string>::iterator endName,
                          SymbolKind kind
                          ) {
        ModuleDefPtr module = context.construct->getModule(moduleName);
        SPUG_CHECK(module,
                   "Unable to find module " << moduleName <<
                    " which contains referenced symbol " <<
                    makeIterPair(curName, endName)
                   );

        list<string>::iterator next = curName;
        ++next;
        NamespacePtr ns = module;
        for (; next != endName; ++curName, ++next) {
            ns = ns->lookUp(*curName);
            SPUG_CHECK(ns,
                       "Path element " << *curName << " in module " <<
                        module << " should be a namespace."
                       );
        }
        return ns->lookUp(*curName);
    }

    VarDefPtr deserializeAliasBody(Deserializer &deser, SymbolKind kind) {
        string moduleName = deser.readString(Serializer::modNameSize,
                                            "module"
                                            );

        list<string> moduleRelativePath;
        FuncDef::Spec spec;
        bool sharedLibSym = false;
        TypeDef::TypeVecObjPtr paramTypes;
        CRACK_PB_BEGIN(deser, 256, optional)
            CRACK_PB_FIELD(3, string) {
                string name =
                    optionalDeser.readString(Serializer::varNameSize, "name");
                moduleRelativePath.push_back(name);
                break;
            }
            CRACK_PB_FIELD(1, string) {
                NestedDeserializer funcDefDeser(optionalDeser, 32,
                                                "shlibFuncDef"
                                                );
                spec.deserialize(funcDefDeser);
                sharedLibSym = true;
                break;
            }
            CRACK_PB_FIELD(2, ref)
                if (!paramTypes)
                    paramTypes = new TypeDef::TypeVecObj();
                paramTypes->push_back(TypeDef::deserializeRef(optionalDeser));
                break;
        CRACK_PB_END

        SPUG_CHECK(!sharedLibSym || kind == K_OVLD,
                   "Symbol " << moduleName << "." <<
                   makeIterPair(moduleRelativePath) <<
                    " is a shared library symbol but we were expecting "
                    "an object of kind = " << kind
                   );

        VarDefPtr varDef;

        if (sharedLibSym) {
            ImportedDefVec symbols;
            SPUG_CHECK(moduleRelativePath.size() == 1,
                       "Got a multi-level path for a shared library import: "
                        << makeIterPair(moduleRelativePath)
                       );
            string name = moduleRelativePath.front();
            symbols.push_back(ImportedDef(name, name));

            // XXX: this is horrible.  This is importing the shared library
            // symbol into a fake namespace and then extracting it from the
            // namespace.  We should probably add a special hook into the
            // builder to do this instead.
            GlobalNamespace ns(0, "");
            deser.context->builder.importSharedLibrary(moduleName,
                                                       symbols,
                                                       *deser.context,
                                                       &ns
                                                       );
            FuncDefPtr funcDef;
            varDef = ns.lookUp(name);
            StubDefPtr stub = StubDefPtr::rcast(varDef);
            if (!stub) {
                OverloadDefPtr ovld = OverloadDefPtr::rcast(varDef);
                funcDef = ovld->getSigMatch(spec.args);
                SPUG_CHECK(funcDef,
                           "non-matching shared library function " <<
                            moduleName << "." << name
                           );
                return ovld;
            }

            funcDef = deser.context->builder.createExternFunc(
                *deser.context, spec.flags, name,
                spec.returnType.get(),
                spec.receiverType.get(),
                spec.args,
                stub->address,
                0  // symbol name, need to persist.
            );

            return stub->getOwner()->replaceDef(funcDef.get());
        }

        varDef = resolvePath(*deser.context, moduleName,
                             moduleRelativePath.begin(),
                             moduleRelativePath.end(),
                             kind);
        if (paramTypes) {
            TypeDefPtr typeDef = varDef;
            varDef = typeDef->getSpecialization(*deser.context, paramTypes.get());
        }
        SPUG_CHECK(varDef,
                    "alias not resolved: " << moduleName << ":" <<
                    makeIterPair(moduleRelativePath)
                   );
        return varDef;
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

VarDefPtr VarDef::deserializeAlias(Deserializer &deser) {
    VarDefPtr result = VarDefPtr::rcast(deser.readObject(AliasReader(),
                                                         "ext"
                                                         ).object
                                        );
    deser.readString(64, "optional");
    return result;
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
    TypeDefPtr type = TypeDef::deserializeRef(deser);
    return deser.context->builder.materializeVar(*deser.context, name,
                                                 type.get(),
                                                 instSlot
                                                 );
}
