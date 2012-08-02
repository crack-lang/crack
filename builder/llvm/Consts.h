// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_Consts_h_
#define _builder_llvm_Consts_h_

#include "model/StrConst.h"
#include "model/IntConst.h"
#include "model/FloatConst.h"

#include <string>

namespace llvm {
    class Value;
    class Module;
}

namespace model {
    class TypeDef;
}

namespace builder {
namespace mvll {

class BTypeDef;

SPUG_RCPTR(BStrConst);

class BStrConst : public model::StrConst {
public:
    // XXX need more specific type?
    llvm::Value *rep;
    llvm::Module *module;
    BStrConst(model::TypeDef *type, const std::string &val);
};
    
class BIntConst : public model::IntConst {
public:
    llvm::Value *rep;
    BIntConst(BTypeDef *type, int64_t val);
    BIntConst(BTypeDef *type, uint64_t val);
    virtual model::IntConstPtr create(int64_t val);
    virtual model::IntConstPtr create(uint64_t val);
};

class BFloatConst : public model::FloatConst {
public:
    llvm::Value *rep;
    BFloatConst(BTypeDef *type, double val);
    virtual model::FloatConstPtr create(double val) const;
};

} // end namespace builder::vmll
} // end namespace builder

#endif
