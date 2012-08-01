// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_GenericParm_h_
#define _model_GenericParm_h_

#include <vector>

#include "spug/RCPtr.h"
#include "spug/RCBase.h"

namespace model {

SPUG_RCPTR(GenericParm);

/** A parameter for a generic type. */
class GenericParm : public spug::RCBase {
    public:
        std::string name;
        GenericParm(const std::string &name) : name(name) {}
};

typedef std::vector<GenericParmPtr> GenericParmVec;

} // namespace model

#endif
