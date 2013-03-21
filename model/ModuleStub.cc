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
            TypeStub(ModuleStub *module, const string &name) :
                TypeDef(0, name),
                module(module) {
            }
    };

    class OverloadStub : public OverloadDef {
        public:
            ModuleStub *module;
            OverloadStub(ModuleStub *module, const string &name) :
                OverloadDef(name),
                module(module) {
            }
    };

    class VarStub : public VarDef {
        public:
            ModuleStub *module;
            VarStub(ModuleStub *module, const string &name) :
                VarDef(0, name),
                module(module) {
            }
    };

} // anon namespace

TypeDefPtr ModuleStub::getTypeStub(const string &name) {
    return new TypeStub(this, name);
}

OverloadDefPtr ModuleStub::getOverloadStub(const string &name) {
    return new OverloadStub(this, name);
}

VarDefPtr ModuleStub::getVarStub(const string &name) {
    return new VarStub(this, name);
}
