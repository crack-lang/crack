// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BBuilderContextData_h_
#define _builder_llvm_BBuilderContextData_h_

#include "model/BuilderContextData.h"
#include <spug/RCPtr.h>

namespace llvm {
    class Function;
    class BasicBlock;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BBuilderContextData);

class BBuilderContextData : public model::BuilderContextData {
public:
    llvm::Function *func;
    llvm::BasicBlock *block;

    BBuilderContextData() :
            func(0),
            block(0) {
    }
};

} // end namespace builder::vmll
} // end namespace builder

#endif
