// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

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

BTypeDefPtr createMetaClass(model::Context &context,
                            const std::string &name,
                            std::vector<model::TypeDefPtr> bases,
                            BGlobalVarDefImplPtr &classImpl
                            );

} // end namespace builder::vmll
} // end namespace builder

#endif
