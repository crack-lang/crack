// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "NamespaceAliasTreeNode.h"

#include "spug/stlutil.h"

#include "Namespace.h"
#include "OverloadDef.h"
#include "Serializer.h"
#include "VarDef.h"

using namespace model;
using namespace std;

void NamespaceAliasTreeNode::serialize(Serializer &serializer) const {
    if (Serializer::trace)
        cerr << "# begin namespace " << ns->getNamespaceName() << endl;
    ns->serializeHeader(serializer);
    serializer.write(children.size(), "#children");
    SPUG_FOR(vector<AliasTreeNodePtr>, i, children)
        (*i)->serialize(serializer);
    serializer.write(aliases.size(), "#defs");
    SPUG_FOR(VarDefMap, i, aliases)
        i->second->serializeAlias(serializer, i->first);
    if (Serializer::trace)
        cerr << "# end namespace " << ns->getNamespaceName() << endl;
}
