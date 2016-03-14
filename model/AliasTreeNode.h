// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_AliasTreeNode_h_
#define _model_AliasTreeNode_h_

#include <map>
#include <string>
#include <vector>

#include "spug/RCBase.h"
#include "spug/RCPtr.h"

namespace model {

SPUG_RCPTR(FuncDef);
SPUG_RCPTR(Namespace);
SPUG_RCPTR(OverloadDef);
class Serializer;
SPUG_RCPTR(VarDef);

SPUG_RCPTR(AliasTreeNode);

// The "alias tree" is a tree of namespaces, overloads and all of the aliases
// that they contain collected for purposes of serialization.
class AliasTreeNode : public spug::RCBase {
    public:
        virtual void serialize(Serializer &serializer) const = 0;
};

SPUG_RCPTR(NamespaceAliasTreeNode);

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

SPUG_RCPTR(OverloadAliasTreeNode);

class OverloadAliasTreeNode : public AliasTreeNode {
    private:
        OverloadDefPtr overload;

        std::vector<FuncDefPtr> aliases;

    public:
        OverloadAliasTreeNode(OverloadDef *overload) : overload(overload) {}

        void addAlias(FuncDef *func);

        virtual void serialize(Serializer &serializer) const;
};

} // namespace model

#endif
