// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_NegOpDef_h_
#define _model_NegOpDef_h_

#include "FuncDef.h"

namespace model {

SPUG_RCPTR(FuncCall);

SPUG_RCPTR(OpDef);

// Base class for primitive operations.
class OpDef : public FuncDef {
    public:

        OpDef(model::TypeDef *resultType, model::FuncDef::Flags flags,
              const std::string &name,
              size_t argCount
              ) :
            FuncDef(flags, name, argCount) {

            // XXX we don't have a function type for these, when this gets
            // fixed search for builtins_need_oper_call and change the
            // relevant code.
            returnType = resultType;
        }

        virtual FuncCallPtr createFuncCall() = 0;

        virtual void *getFuncAddr(builder::Builder &builder) {
            return 0;
        }
};

}  // namespace model

#endif
