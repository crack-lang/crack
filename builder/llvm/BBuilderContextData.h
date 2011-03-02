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

class IncompleteCatchSelector;

SPUG_RCPTR(BBuilderContextData);

class BBuilderContextData : public model::BuilderContextData {
public:
    // state information on the current try/catch block.
    struct CatchData {
        // list of the implementation values of the classes in "catch" clauses
        std::vector<llvm::Value *> classImpls;
        
        // list of the incomplete catch selectors for the block.
        std::vector<IncompleteCatchSelector *> selectors;
    };

    llvm::Function *func;
    llvm::BasicBlock *block,
        *unwindBlock,      // the unwind block for the catch/function
        *nextCleanupBlock; // the current next cleanup block

    BBuilderContextData() :
            func(0),
            block(0),
            unwindBlock(0),
            nextCleanupBlock(0),
            catchData(0) {
    }
    
    ~BBuilderContextData() {
        delete catchData;
    }
    
    static BBuilderContextData *get(model::Context *context) {
        if (!context->builderData)
            context->builderData = new BBuilderContextData();
        return BBuilderContextDataPtr::rcast(context->builderData);
    }
    
    CatchData &getCatchData() {
        if (!catchData)
            catchData = new CatchData();
        return *catchData;
    }
    
    void deleteCatchData() {
        assert(catchData);
        delete catchData;
        catchData = 0;
    }

private:
    CatchData *catchData;
    BBuilderContextData(const BBuilderContextData &);
};

}} // end namespace builder::vmll

#endif
