// Copyright 2009 Google Inc.

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
class Branchpoint : public spug::RCBase { };

} // namespace model

#endif
