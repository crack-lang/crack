// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "OverloadAliasTreeNode.h"

#include "spug/stlutil.h"

#include "OverloadDef.h"
#include "Serializer.h"
#include "VarDef.h"

using namespace model;
using namespace std;

void OverloadAliasTreeNode::addAlias(FuncDef *func) {
    aliases.push_back(func);
}

void OverloadAliasTreeNode::serialize(Serializer &serializer) const {
    if (Serializer::trace)
        cerr << "# begin overload " << overload->name << endl;
    serializer.write(Serializer::overloadId, "kind");
    serializer.write(overload->name, "name");
    serializer.write(aliases.size(), "#defs");
    SPUG_FOR(vector<FuncDefPtr>, i, aliases)
        // XXX serializeAlias no longer needs to be virtual, we don't use the
        // name (i->first) in the overload version.
        (*i)->serializeAlias(serializer);
    if (Serializer::trace)
        cerr << "# end overload " << overload->name << endl;
}

