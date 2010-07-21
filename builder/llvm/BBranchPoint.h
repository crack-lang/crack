// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BBranchPoint_h_
#define _builder_llvm_BBranchPoint_h_

#include "spug/RCPtr.h"
#include "model/Branchpoint.h"

namespace llvm {
    class BasicBlock;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BBranchpoint);

class BBranchpoint : public model::Branchpoint {
public:
    llvm::BasicBlock *block, *block2;

    BBranchpoint(llvm::BasicBlock *block) : block(block), block2(0) {}
};

} // end namespace builder::vmll
} // end namespace builder

#endif
