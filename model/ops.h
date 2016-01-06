// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ops_h_
#define _model_ops_h_

#include "FuncCall.h"
#include "OpDef.h"

namespace model {

class NegOpCall : public model::FuncCall {
    public:
        NegOpCall(model::FuncDef *def) : FuncCall(def) {}
        virtual model::ExprPtr foldConstants();
};

class NegOpDef : public OpDef {
    public:
        NegOpDef(TypeDef *resultType, const std::string &name,
                 bool isMethod
                 );
};

}  // namespace model

#endif
