// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "BCleanupFrame.h"

#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include "model/ResultExpr.h"
#include "model/VarDef.h"
#include "BBuilderContextData.h"
#include "BTypeDef.h"
#include "VarDefs.h"
#include "Incompletes.h"
#include "LLVMBuilder.h"

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder::mvll;

void BCleanupFrame::close() {
    if (dynamic_cast<LLVMBuilder &>(context->builder).suppressCleanups())
        return;
    context->emittingCleanups = true;
    for (CleanupList::iterator iter = cleanups.begin();
         iter != cleanups.end();
         ++iter
         ) {
        iter->emittingCleanups = true;
        iter->action->emit(*context);
        iter->emittingCleanups = false;
    }
    context->emittingCleanups = false;
}

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
        // if we're already emitting this cleanup in a normal context, stop
        // here
        if (iter->emittingCleanups)
            break;

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

BasicBlock *BCleanupFrame::getLandingPad(
    BasicBlock *next,
    BBuilderContextData::CatchData *cdata
) {
    // if this is an empty frame, associate the landing pad with the frame.
    // Otherwise, associate it with the first cleanup
    BasicBlock *&lp =
        (cleanups.size()) ? cleanups.front().landingPad : landingPad;

    if (!lp) {
        // get the builder
        LLVMBuilder &llvmBuilder =
            dynamic_cast<LLVMBuilder &>(context->builder);

        lp = llvmBuilder.createLandingPad(*context, next, cdata);
    }

    return lp;
}

void BCleanupFrame::clearCachedCleanups() {
    for (CleanupList::iterator iter = cleanups.begin(); iter != cleanups.end();
         ++iter
         )
        iter->unwindBlock = iter->landingPad = 0;
    landingPad = 0;
    if (parent)
        BCleanupFramePtr::rcast(parent)->clearCachedCleanups();
}
