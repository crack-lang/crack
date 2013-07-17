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
#include <vector>

#include "NamespaceStub.h"
// we need to include this because of TypeDef::TypeVecObj
#include "TypeDef.h"

namespace model {

class Context;
SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(ModuleStub);

class ModuleStub : public ModuleDef, public NamespaceStub {
    public:

        struct Callback {
            virtual void run(Context &context) = 0;
        };

        typedef std::vector<Callback *> CallbackVec;
        CallbackVec callbacks;

        bool replacedAll;

        // modules that depend on the stub and need to be fixed.
        std::set<ModuleDef *> dependents;

        ModuleStub(const std::string &name) :
            ModuleDef(name, 0),
            replacedAll(false) {
        }

        ~ModuleStub();

        virtual void callDestructor() {}
        virtual void runMain(builder::Builder &builder) {}

        // Implements NamespaceStub.
        virtual TypeDefPtr getTypeStub(const std::string &name);
        virtual OverloadDefPtr getOverloadStub(const std::string &name);
        virtual VarDefPtr getVarStub(const std::string &name);
        virtual NamespaceStubPtr getTypeNSStub(const std::string &name);
        virtual Namespace *getRealNamespace();
        virtual ModuleDefPtr getModule();

        /**
         * Replace this stub in all modules in 'dependents' with 'module'.
         */
        void replace(Context &context, ModuleDef *replacement);

        virtual TypeDefPtr getType(const std::string &name);

        /**
         * Registers the callback to be called after the stub is replaced.
         * Ownership of the callback is transferred to the ModuleStub.
         */
        void registerCallback(Callback *callback);

        /**
         * Returns a stubbed type for a generic with stubbed parameters.
         * @param dependent The module that depends on the new stub.
         * @param stub The first stubbed parameter.
         * @param generic the original generic type.
         * @param types the generic's type parameters.
         */
        static TypeDefPtr createGenericStub(ModuleDef *dependent,
                                            TypeDef *stub,
                                            TypeDef *generic,
                                            TypeDef::TypeVecObj *types
                                            );
};

} // namespace model

#endif
