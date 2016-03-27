// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_AliasTreeNode_h_
#define _model_AliasTreeNode_h_

#include "spug/RCBase.h"
#include "spug/RCPtr.h"

namespace model {

class Serializer;

SPUG_RCPTR(AliasTreeNode);

// The "alias tree" is a tree of namespaces, overloads and all of the aliases
// that they contain collected for purposes of serialization.
class AliasTreeNode : public spug::RCBase {
    public:
        virtual void serialize(Serializer &serializer) const = 0;
};

} // namespace model

#endif
