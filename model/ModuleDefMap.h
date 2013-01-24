// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ModuleDefMap_h_
#define _model_ModuleDefMap_h_

#include <map>
#include "spug/RCPtr.h"

namespace model {

SPUG_RCPTR(ModuleDef);

class ModuleDefMap : public std::map<std::string, ModuleDefPtr> {
    public:
        bool hasKey(const std::string &name) {
            return find(name) != end();
        }
};

} // namespace model

#endif
