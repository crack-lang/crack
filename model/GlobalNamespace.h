// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_GlobalNamespace_h_
#define _model_GlobalNamespace_h_

#include "LocalNamespace.h"

namespace model {

SPUG_RCPTR(GlobalNamespace);

class GlobalNamespace : public LocalNamespace {
    public:

        GlobalNamespace(Namespace *parent, const std::string &cName) :
            LocalNamespace(parent, cName) {
        }
};

} // namespace model

#endif
