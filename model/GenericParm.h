// Copyright 2011 Google Inc.

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
