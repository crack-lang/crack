// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ArgDef.h"

#include "spug/check.h"
#include "builder/Builder.h"
#include "Context.h"
#include "Serializer.h"
#include "Deserializer.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

void ArgDef::serialize(Serializer &serializer, bool writeKind,
                       const Namespace *ns
                       ) const {
    SPUG_CHECK(!writeKind, "?? writing 'kind' for arg variable " << name);
    serializer.write(name, "name");
    type->serialize(serializer, false, 0);
    serializer.write(0, "optional");
}


ArgDefPtr ArgDef::deserialize(Deserializer &deser) {
    string name = deser.readString(16, "name");
    TypeDefPtr type = TypeDef::deserializeRef(deser);
    deser.readString(64, "optional");
    return deser.context->builder.materializeArg(*deser.context, name,
                                                 type.get()
                                                 );
}
