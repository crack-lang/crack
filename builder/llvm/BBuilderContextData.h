// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BBuilderContextData_h_
#define _builder_llvm_BBuilderContextData_h_

#include "model/BuilderContextData.h"
#include <spug/RCPtr.h>

namespace llvm {
    class Function;
    class BasicBlock;
}

namespace builder { namespace mvll {

SPUG_RCPTR(BBuilderContextData);

class BBuilderContextData : public model::BuilderContextData {
public:
    llvm::Function *func;
    llvm::BasicBlock *block,
        *unwindBlock,      // the unwind block for the catch/function
        *nextCleanupBlock; // the current next cleanup block

    BBuilderContextData() :
            func(0),
            block(0),
            unwindBlock(0),
            nextCleanupBlock(0) {
    }
    
    static BBuilderContextData *get(model::Context *context) {
        if (!context->builderData)
            context->builderData = new BBuilderContextData();
        return BBuilderContextDataPtr::rcast(context->builderData);
    }
};

}} // end namespace builder::vmll

#endif
