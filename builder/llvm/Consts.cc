// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Consts.h"
#include "BTypeDef.h"
#include <llvm/IR/Constants.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

BStrConst::BStrConst(TypeDef *type, const std::string &val) :
        StrConst(type, val),
        rep(0),
        module(0) {
}
    
BIntConst::BIntConst(BTypeDef *type, int64_t val) :
        IntConst(type, val),
        rep(ConstantInt::get(type->rep, val)) {
}

IntConstPtr BIntConst::create(int64_t val) {
    return new BIntConst(BTypeDefPtr::arcast(type), val);
}

IntConstPtr BIntConst::create(uint64_t val) {
    return new BIntConst(BTypeDefPtr::arcast(type), val);
}

BIntConst::BIntConst(BTypeDef *type, uint64_t val) :
        IntConst(type, val),
        rep(ConstantInt::get(type->rep, val)) {
}

BFloatConst::BFloatConst(BTypeDef *type, double val) :
        FloatConst(type, val),
        rep(ConstantFP::get(type->rep, val)) {
}

FloatConstPtr BFloatConst::create(double val) const {
    return new BFloatConst(BTypeDefPtr::arcast(type), val);
}
