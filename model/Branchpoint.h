// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Branchpoint_h_
#define _model_Branchpoint_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(Branchpoint);

/**
 * An opaque datastructure for storing the location of a branchpoint that is 
 * likely to need to be fixed up by the builder.
 */
class Branchpoint : public spug::RCBase {
    public:
        Context *context;
        Branchpoint(Context *context) : context(context) {}
        Branchpoint() : context(0) {}
};

} // namespace model

#endif
