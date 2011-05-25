// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BFuncPtr_h_
#define _builder_llvm_BFuncPtr_h_

#include "model/FuncDef.h"
#include "llvm/Value.h"

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
    model::FuncDef(FuncDef::noFlags, r->getNameStr(), argCount),
    rep(r) {    }

    virtual void *getFuncAddr(builder::Builder &builder) {
        assert(0 && "getFuncAddr called on BFuncPtr");
    }

};

} // end namespace builder::vmll
} // end namespace builder

#endif
