// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleStub.h"

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
                TypeVecObjKey key(params);
                if (!generic) {
                    generic = new SpecializationCache();
                } else {
                    SpecializationCache::iterator i = generic->find(key);
                    if (i != generic->end())
                        return i->second.get();
                }
                TypeDef *result;
                (*generic)[key] = result = new TypeStub(module, name, params);
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

            VarDefPtr replaceStub(Context &context) {
                return module->replacement->lookUp(name);
            }
    };

} // anon namespace

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
}
