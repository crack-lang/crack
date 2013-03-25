// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ModulePlaceholder_h_
#define _model_ModulePlaceholder_h_

#include "ModuleDef.h"

#include <set>

namespace model {

class Context;
SPUG_RCPTR(Overload);
SPUG_RCPTR(Type);
SPUG_RCPTR(ModuleStub);

class ModuleStub : public ModuleDef {
    public:
        ModuleDefPtr replacement;

        // modules that depend on the stub and need to be fixed.
        std::set<ModuleDef *> dependents;

        ModuleStub(const std::string &name) :
            ModuleDef(name, 0) {
        }

        virtual void callDestructor() {}
        virtual void runMain(builder::Builder &builder) {}

        /**
         * These functions get placeholders for symbols defined inside the
         * module.
         */
        /** @{ */
        TypeDefPtr getTypeStub(const std::string &name);
        OverloadDefPtr getOverloadStub(const std::string &name);
        VarDefPtr getVarStub(const std::string &name);
        /** @} */

        /**
         * Replace this stub in all modules in 'dependents'.
         */
        void replace(Context &context);
};

} // namespace model

#endif
