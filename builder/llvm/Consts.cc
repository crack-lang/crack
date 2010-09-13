// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "Consts.h"
#include "BTypeDef.h"
#include <llvm/Constants.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

BStrConst::BStrConst(TypeDef *type, const std::string &val) :
        StrConst(type, val),
        rep(0) {
}
    
BIntConst::BIntConst(BTypeDef *type, int64_t val) :
        IntConst(type, val),
        rep(ConstantInt::get(type->rep, val)) {
}
BIntConst::BIntConst(BTypeDef *type, uint64_t val) :
        IntConst(type, val),
        rep(ConstantInt::get(type->rep, val)) {
}

BFloatConst::BFloatConst(BTypeDef *type, double val) :
        FloatConst(type, val),
        rep(ConstantFP::get(type->rep, val)) {
}

