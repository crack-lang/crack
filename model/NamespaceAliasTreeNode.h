// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_NamespaceAliasTreeNode_h_
#define _model_NamespaceAliasTreeNode_h_

#include <map>
#include <vector>

#include "AliasTreeNode.h"

namespace model {

SPUG_RCPTR(Namespace);
class Serializer;
SPUG_RCPTR(VarDef);

SPUG_RCPTR(NamespaceAliasTreeNode);

// Alias tree node for representing namespaces (types and modules,
// specifically).
class NamespaceAliasTreeNode : public AliasTreeNode {
    private:
        NamespacePtr ns;
        std::vector<AliasTreeNodePtr> children;

        typedef std::map<std::string, VarDefPtr> VarDefMap;
        VarDefMap aliases;

    public:
        NamespaceAliasTreeNode(Namespace *ns) : ns(ns) {}

        void addChild(AliasTreeNode *child) {
            children.push_back(child);
        }

        void addAlias(const std::string &name, VarDef *def) {
            aliases[name] = def;
        }

        virtual void serialize(Serializer &serializer) const;
};

}

#endif
