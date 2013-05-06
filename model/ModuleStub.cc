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

    class TypeStub : public TypeDef {
        public:
            ModuleStub *module;
            TypeVecObjPtr params;
            TypeStub(ModuleStub *module, const string &name,
                     TypeVecObj *params
                     ) :
                TypeDef(0, name),
                module(module),
                params(params) {

                owner = module;
            }

            virtual bool isStub() const { return true; }

            VarDefPtr replaceStub(Context &context) {
                TypeDefPtr replacement = module->replacement->lookUp(name);
                if (params)
                    return replacement->getSpecialization(context,
                                                          params.get()
                                                          );
                else
                    return replacement;
            }

            TypeDef *getSpecialization(Context &context,
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

    class OverloadStub : public OverloadDef {
        public:
            ModuleStub *module;
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
            ModuleStub *module;
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
    return new TypeStub(this, name, 0);
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
}

void ModuleStub::registerCallback(ModuleStub::Callback *callback) {
    callbacks.push_back(callback);
}