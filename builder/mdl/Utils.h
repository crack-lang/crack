// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_mdl_Utils_h_
#define _builder_mdl_Utils_h_

#include <string>

#include "model/FuncDef.h"

namespace model {
    class Context;
    class TypeDef;
}

namespace builder { namespace mdl { namespace util {

model::FuncDefPtr newFuncDef(model::TypeDef *returnType,
                             model::FuncDef::Flags flags,
                             const std::string &name,
                             size_t argCount
                             );

model::FuncDefPtr newUnOpDef(model::TypeDef *returnType,
                             const std::string &name,
                             bool isMethod
                             );

model::FuncDefPtr newBinOpDef(const std::string &name, model::TypeDef *argType,
                              model::TypeDef *returnType,
                              bool isMethod = false,
                              bool isReversed = false
                              );

void addConvNew(model::Context &context, model::TypeDef *source,
                model::TypeDef *target
                );

void addNopNew(model::Context &context, model::TypeDef *type);

model::FuncDefPtr newVoidPtrOpDef(model::TypeDef *resultType);

}}}

#endif
