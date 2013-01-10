// Copyright 2009-2010,2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ArgDef_h_
#define _model_ArgDef_h_

#include <vector>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(TypeDef);

SPUG_RCPTR(ArgDef);

class ArgDef : public VarDef {
    public:
        ArgDef(TypeDef *type, const std::string &name) :
            VarDef(type, name) {
        }
        void serialize(Serializer &serializer, bool writeKind,
                       const Namespace *ns
                       ) const;
        static ArgDefPtr deserialize(Deserializer &deser);
};

typedef std::vector<ArgDefPtr> ArgVec;

} // namespace model

#endif
