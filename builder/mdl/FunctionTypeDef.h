// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_FunctionTypeDef_h_
#define _builder_llvm_FunctionTypeDef_h_

#include "model/Context.h"
#include "model/TypeDef.h"
#include <string>

namespace llvm {
    class Type;
}

namespace builder {
namespace mdl {

class FunctionTypeDef : public model::TypeDef {
public:
    FunctionTypeDef(model::TypeDef *metaType,
                    const std::string &name
                    );

    // specializations of array types actually create a new type
    // object.
    virtual model::TypeDefPtr getSpecialization(model::Context &context,
                                                TypeVecObj *types
                                                );

};


} // end namespace builder::vmll
} // end namespace builder

#endif
