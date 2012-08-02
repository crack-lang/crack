// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_BuilderContextData_h_
#define _model_BuilderContextData_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(BuilderContextData);

/**
 * Base class for Builder-specific information to be stored in a Context 
 * object.
 */
class BuilderContextData : public spug::RCBase {
    public:
        BuilderContextData() {}
};

} // namespace model

#endif
