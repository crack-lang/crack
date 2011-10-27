// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Consts_h_
#define _builder_llvm_Consts_h_

#include "model/StrConst.h"
#include "model/IntConst.h"
#include "model/FloatConst.h"

#include <string>

namespace llvm {
    class Value;
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
};

} // end namespace builder::vmll
} // end namespace builder

#endif
