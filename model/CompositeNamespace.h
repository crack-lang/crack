// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_CompositeNamespace_h_
#define _model_CompositeNamespace_h_

#include <vector>
#include "Namespace.h"

namespace model {

SPUG_RCPTR(CompositeNamespace);
SPUG_RCPTR(OverloadDef);

/**
 * A virtual namespace that delegates to several other namespaces.
 *
 * All new definitions added to the namespace will be stored in the first
 * parent.
 */
class CompositeNamespace : public Namespace {
    private:
        std::vector<NamespacePtr> parents;

    public:
        CompositeNamespace(Namespace *parent0, Namespace *parent1);
        virtual ModuleDefPtr getModule();
        virtual bool isHiddenScope();
        virtual NamespacePtr getParent(unsigned index);
        virtual void addDef(VarDef *def);
        virtual void removeDef(VarDef *def);
        virtual void addAlias(VarDef *def);
        virtual OverloadDefPtr addAlias(const std::string &name, VarDef *def);
        virtual void addUnsafeAlias(const std::string &name, VarDef *def);
        virtual OverloadDefPtr replaceDef(VarDef *def);
};

} // namespace model

#endif
