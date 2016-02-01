// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_BinOpDef_h_
#define _model_BinOpDef_h_

#include "OpDef.h"

namespace model {

SPUG_RCPTR(BinOpDef);

// Base class for binary operators.
// Derived classes must implement:
//     model::FuncCallPtr createFuncCall()
//     model::ResultExprPtr emit(Context &context)
class BinOpDef : public OpDef {
    public:
        BinOpDef(model::TypeDef *argType,
                 model::TypeDef *resultType,
                 const std::string &name,
                 bool isMethod = false,
                 bool reversed = false
                 );

        virtual model::FuncCallPtr createFuncCall() = 0;
};

}  // namespace model

#endif
