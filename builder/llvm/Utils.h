// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_Utils_h_
#define _builder_llvm_Utils_h_

#include "model/Context.h"
#include "BTypeDef.h"
#include "VarDefs.h"

#include <string>
#include <vector>

namespace builder {
namespace mvll {

void addArrayMethods(model::Context &context,
                     model::TypeDef *arrayType,
                     BTypeDef *elemType
                     );

void closeAllCleanupsStatic(model::Context &context);

/**
 * Create the implementation object for a class.
 */
void createClassImpl(model::Context &context, BTypeDef *type);

/**
 * Creates a new meta-class and registers it with its owner.  If no owner is
 * specified, the owner is the first definition context of the /parent/ of
 * 'context' (i.e. 'context' is assumed to be a class context).
 */
BTypeDefPtr createMetaClass(model::Context &context,
                            const std::string &name,
                            model::Namespace *owner = 0
                            );

llvm::Value *createInvoke(llvm::IRBuilder<> &builder, model::Context &context,
                          llvm::Value *func,
                          std::vector<llvm::Value *> &valueArgs
                          );

} // end namespace builder::vmll
} // end namespace builder

#endif
