// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_BFuncPtr_h_
#define _builder_llvm_BFuncPtr_h_

#include "model/FuncDef.h"
#include "llvm/IR/Value.h"

namespace llvm {
    class Function;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BFuncPtr);

class BFuncPtr : public model::FuncDef {
public:

    // this holds the function pointer
    llvm::Value *rep;

    BFuncPtr(llvm::Value *r, size_t argCount) :
    model::FuncDef(FuncDef::noFlags, r->getName(), argCount),
    rep(r) {    }

    virtual void *getFuncAddr(builder::Builder &builder) {
        assert(0 && "getFuncAddr called on BFuncPtr");
    }

};

} // end namespace builder::vmll
} // end namespace builder

#endif
