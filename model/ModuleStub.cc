// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleStub.h"

#include "spug/check.h"
#include "Context.h"
#include "OverloadDef.h"

using namespace std;
using namespace model;

namespace {

    // Base class for type stubs.
    class TypeStub : public TypeDef {
        public:

            // the is the module that needs to be resolved for the type stub
            // to get resolved.  In the case of a PrimaryTypeStub, it is also
            // the stub module that owns the type.
            ModuleStubPtr module;

            // The type parameters.  We need these for both subclasses.
            TypeVecObjPtr params;

            TypeStub(ModuleStub *module, const string &name,
                     TypeVecObj *params
                     ) :
                TypeDef(0, name),
                module(module),
                params(params) {
            }

            virtual bool isStub() const { return true; }
    };

    // A type stub in which the primary type itself is stubbed.
    class PrimaryTypeStub : public TypeStub {
        public:
            PrimaryTypeStub(ModuleStub *module, const string &name,
                            TypeVecObj *params
                            ) :
                TypeStub(module, name, params) {

                owner = module;
            }

            VarDefPtr replaceStub(Context &context) {
                TypeDefPtr replacement = module->replacement->lookUp(name);
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

            GenericTypeStub(ModuleStub *module, TypeDef *realType,
                            TypeVecObj *params
                            ) :
                TypeStub(module, makeName(realType, params), params),
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
    };

    class OverloadStub : public OverloadDef {
        public:
            ModuleStubPtr module;
            OverloadStub(ModuleStub *module, const string &name) :
                OverloadDef(name),
                module(module) {
            }

            bool isStub() const {
                return true;
            }

            VarDefPtr replaceStub(Context &context) {
                return module->replacement->lookUp(name);
            }
    };

    class VarStub : public VarDef {
        public:
            ModuleStubPtr module;
            VarStub(ModuleStub *module, const string &name) :
                VarDef(0, name),
                module(module) {
            }

            bool isStub() const {
                return true;
            }

            VarDefPtr replaceStub(Context &context) {
                return module->replacement->lookUp(name);
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
    ModuleStubPtr stubMod = dynamic_cast<TypeStub *>(stub)->module;
    SPUG_CHECK(!stubMod->replacedAll,
               "Module " << stubMod->name <<
                " should have already replaced all stubs."
               );
    stubMod->dependents.insert(dependent);
    return new GenericTypeStub(stubMod.get(), generic, types);
}
