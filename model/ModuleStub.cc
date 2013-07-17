// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleStub.h"

#include "spug/check.h"
#include "Context.h"
#include "NamespaceStub.h"
#include "OverloadDef.h"

using namespace std;
using namespace model;

namespace {

    class OverloadStub : public OverloadDef {
        public:
            NamespaceStubPtr ns;
            OverloadStub(NamespaceStub *ns, const string &name) :
                OverloadDef(name),
                ns(ns) {
            }

            bool isStub() const {
                return true;
            }

            VarDefPtr replaceStub(Context &context) {
                return ns->replacement->lookUp(name);
            }
    };

    class VarStub : public VarDef {
        public:
            NamespaceStubPtr ns;
            VarStub(NamespaceStub *ns, const string &name) :
                VarDef(0, name),
                ns(ns) {
            }

            bool isStub() const {
                return true;
            }

            VarDefPtr replaceStub(Context &context) {
                return ns->replacement->lookUp(name);
            }
    };

    // Base class for type stubs.
    class TypeStub : public TypeDef, public NamespaceStub {
        public:

            // the is the namespace that needs to be resolved for the type stub
            // to get resolved.  In the case of a PrimaryTypeStub, it is also
            // the stub namespace that owns the type.
            NamespaceStubPtr ns;

            // The type parameters.  We need these for both subclasses.
            TypeVecObjPtr params;

            TypeStub(NamespaceStub *module, const string &name,
                     TypeVecObj *params
                     ) :
                TypeDef(0, name),
                ns(module),
                params(params) {
            }

            virtual bool isStub() const { return true; }

            virtual TypeDefPtr getTypeStub(const std::string &name);

            virtual OverloadDefPtr getOverloadStub(const std::string &name) {
                return new OverloadStub(this, name);
            }

            virtual VarDefPtr getVarStub(const std::string &name) {
                return new VarStub(this, name);
            }

            virtual NamespaceStubPtr getTypeNSStub(const std::string &name);

            virtual Namespace *getRealNamespace() {
                return this;
            }

            virtual ModuleDefPtr getModule() {
                return ns->getModule();
            }
    };

    // A type stub in which the primary type itself is stubbed.
    class PrimaryTypeStub : public TypeStub {
        public:
            PrimaryTypeStub(NamespaceStub *ns, const string &name,
                            TypeVecObj *params
                            ) :
                TypeStub(ns, name, params) {

                setOwner(ns->getRealNamespace());
            }

            VarDefPtr replaceStub(Context &context) {
                TypeDefPtr replacement = ns->replacement->lookUp(name);
                if (params)
                    return replacement->getSpecialization(context,
                                                          params.get()
                                                          );
                else
                    return replacement;
            }

            TypeDefPtr getSpecialization(Context &context,
                                         TypeVecObj *params
                                         ) {
                // make sure we have a specialization cache
                if (!generic)
                    generic = new SpecializationCache();

                TypeDef *result = findSpecialization(params);
                if (!result) {
                    // For a generic instantiation, we should always either be
                    // able to load it from the cache or reconstruct it from
                    // its source file.
                    string moduleName = getSpecializedName(params, true);
                    ModuleDefPtr mod = context.construct->getModule(moduleName);
                    SPUG_CHECK(mod,
                               "Unable to load generic instantiation "
                                "module " << moduleName
                               );
                    result = TypeDefPtr::rcast(mod->lookUp(name));
                    result->genericParms = *params;
                    result->templateType = this;
                    (*generic)[params] = result;
                }
                return result;
            }
    };

    TypeDefPtr TypeStub::getTypeStub(const std::string &name) {
        return new PrimaryTypeStub(this, name, 0);
    }

    NamespaceStubPtr TypeStub::getTypeNSStub(const std::string &name) {
        return new PrimaryTypeStub(ns.get(), name, 0);
    }

    // Stub for a generic type that is not itself a stub but has stub arguments.
    class GenericTypeStub : public TypeStub {
        public:

            // the actual underlying generic type.
            TypeDefPtr realType;

            static string makeName(TypeDef *type, TypeVecObj *params) {
                string base = type->getSpecializedName(params, true);
                string typeFullName = type->getFullName();
                if (typeFullName != ".builtin.array" &&
                    typeFullName != ".builtin.function"
                    )
                    base += "." + type->name;
                return base;
            }

            GenericTypeStub(NamespaceStub *ns, TypeDef *realType,
                            TypeVecObj *params
                            ) :
                TypeStub(ns, makeName(realType, params), params),
                realType(realType) {
            }

            virtual bool isStub() const { return true; }

            VarDefPtr replaceStub(Context &context) {

                // replace all of the stub params, keep track of whether any of
                // them were not replaced.
                bool stillAStub = false;
                for (TypeVecObj::iterator i = params->begin();
                     i != params->end();
                     ++i
                     ) {
                    *i = (*i)->replaceStub(context);
                    if ((*i)->isStub())
                        stillAStub = true;
                }

                if (stillAStub)
                    return this;
                else
                    return realType->getSpecialization(context, params.get());
            }

            TypeDefPtr getSpecialization(Context &context, TypeVecObj *params) {
                SPUG_CHECK(false,
                        "getSpecialization() called on GenericTypeStub " <<
                            name
                        );
            }

            virtual ModuleDefPtr getModule() {
                SPUG_CHECK(false,
                           "getModule() called on GenericTypeStub " <<
                            name
                           );
            }
    };

} // anon namespace

ModuleStub::~ModuleStub() {
    for (CallbackVec::iterator iter = callbacks.begin();
         iter != callbacks.end();
         ++iter
         )
        delete *iter;
}

TypeDefPtr ModuleStub::getTypeStub(const string &name) {
    return new PrimaryTypeStub(this, name, 0);
}

OverloadDefPtr ModuleStub::getOverloadStub(const string &name) {
    return new OverloadStub(this, name);
}

VarDefPtr ModuleStub::getVarStub(const string &name) {
    return new VarStub(this, name);
}

NamespaceStubPtr ModuleStub::getTypeNSStub(const std::string &name) {
    return new PrimaryTypeStub(this, name, 0);
}

Namespace *ModuleStub::getRealNamespace() {
    return this;
}

ModuleDefPtr ModuleStub::getModule() {
    return this;
}

void ModuleStub::replace(Context &context) {
    for (std::set<ModuleDef *>::iterator iter = dependents.begin();
         iter != dependents.end();
         ++iter
         )
        (*iter)->replaceAllStubs(context);

    for (CallbackVec::iterator iter = callbacks.begin();
         iter != callbacks.end();
         ++iter
         )
        (*iter)->run();
    replacedAll = true;
}

TypeDefPtr ModuleStub::getType(const string &name) {
    return getTypeStub(name);
}

void ModuleStub::registerCallback(ModuleStub::Callback *callback) {
    callbacks.push_back(callback);
}

TypeDefPtr ModuleStub::createGenericStub(ModuleDef *dependent,
                                         TypeDef *stub,
                                         TypeDef *generic,
                                         TypeDef::TypeVecObj *types
                                         ) {
    ModuleStubPtr stubMod = dynamic_cast<TypeStub *>(stub)->ns->getModule();
    SPUG_CHECK(!stubMod->replacedAll,
               "Module " << stubMod->name <<
                " should have already replaced all stubs."
               );
    stubMod->dependents.insert(dependent);
    return new GenericTypeStub(stubMod.get(), generic, types);
}
