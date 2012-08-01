// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
    llvm::BasicBlock *block, *block2, *block3;

    BBranchpoint(llvm::BasicBlock *block) :
        block(block), 
        block2(0),
        block3(0) {
    }
};

} // end namespace builder::vmll
} // end namespace builder

#endif
