// Copyright 2011 Google Inc.

#include "BCleanupFrame.h"

#include "BBuilderContextData.h"
#include "model/ResultExpr.h"
#include "LLVMBuilder.h"
#include <llvm/LLVMContext.h>

using namespace llvm;
using namespace builder::mvll;

BasicBlock *BCleanupFrame::emitUnwindCleanups(BasicBlock *next) {
    context->emittingCleanups = true;
    BBuilderContextData *bdata = BBuilderContextData::get(context);
    bdata->nextCleanupBlock = next;
    
    // store the current block and get a reference to the builder so we 
    // can set the current block
    LLVMBuilder &llvmBuilder = dynamic_cast<LLVMBuilder &>(context->builder);
    IRBuilder<> &builder = 
        llvmBuilder.builder;
    BasicBlock *savedBlock = builder.GetInsertBlock();
    
    // emit these cleanups from back to front
    for (CleanupList::reverse_iterator iter = cleanups.rbegin();
         iter != cleanups.rend();
         ++iter
         ) {
        if (!iter->unwindBlock) {
            iter->unwindBlock = BasicBlock::Create(getGlobalContext(),
                                                   "cleanup",
                                                   llvmBuilder.func
                                                   );
            builder.SetInsertPoint(iter->unwindBlock);

            // emit the cleanup, which should end on an invoke that takes us to 
            // the next block, so a branch should never be needed.
            iter->action->emit(*context);
        }

        bdata->nextCleanupBlock = next = iter->unwindBlock;
    }
    bdata->nextCleanupBlock = 0;
    context->emittingCleanups = false;

    builder.SetInsertPoint(savedBlock);
    return next;
}

void BCleanupFrame::clearCachedCleanups() {
    for (CleanupList::iterator iter = cleanups.begin(); iter != cleanups.end();
         ++iter
         )
        iter->unwindBlock = 0;
    if (parent)
        BCleanupFramePtr::rcast(parent)->clearCachedCleanups();
}

