// Copyright 2009 Google Inc.

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
};

typedef std::vector<ArgDefPtr> ArgVec;

} // namespace model

#endif
