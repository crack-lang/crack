// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_OverloadAliasTreeNode_h_
#define _model_OverloadAliasTreeNode_h_

#include <map>
#include <vector>

#include "AliasTreeNode.h"

namespace model {

SPUG_RCPTR(FuncDef);
SPUG_RCPTR(OverloadDef);
class Serializer;

SPUG_RCPTR(OverloadAliasTreeNode);

// Alias tree node for representing overloads.
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
