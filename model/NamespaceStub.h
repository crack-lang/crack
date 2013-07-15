// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_NamespaceStub_h_
#define _model_NamespaceStub_h_

#include "spug/RCBase.h"
#include "spug/RCPtr.h"

namespace model {

SPUG_RCPTR(NamespaceStub);

SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDef);

// An interface for stubs that can nest other symbols (modules and types).
class NamespaceStub : public virtual spug::RCBase {
    public:
        NamespacePtr replacement;

        /**
         * These functions get placeholders for symbols defined inside the
         * module.
         */
        /** @{ */
        virtual TypeDefPtr getTypeStub(const std::string &name) = 0;
        virtual OverloadDefPtr getOverloadStub(const std::string &name) = 0;
        virtual VarDefPtr getVarStub(const std::string &name) = 0;

        /**
         * Returns a namespace stub for a type.
         */
        virtual NamespaceStubPtr getTypeNSStub(const std::string &name) = 0;
        /** @} */

        /**
         * Returns the real Namespace instance associated with the namespace
         * type.  This should normally be a cross-conversion.
         */
        virtual Namespace *getRealNamespace() = 0;

        /**
         * Returns the module containing the namespace.
         */
        virtual ModuleDefPtr getModule() = 0;
};

} // namespace model

#endif

