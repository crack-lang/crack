// Copyright 2011 Google Inc.

#include "BCleanupFrame.h"

#include <llvm/Intrinsics.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
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

    // look up the exception structure variable and get its type
    VarDefPtr exStructVar = context->ns->lookUp(":exStruct");
    Type *exStructType = BTypeDefPtr::arcast(exStructVar->type)->rep;

    if (!lp) {
        // get the builder
        LLVMBuilder &llvmBuilder =
            dynamic_cast<LLVMBuilder &>(context->builder);

        lp = BasicBlock::Create(getGlobalContext(), "lp", llvmBuilder.func);
        IRBuilder<> b(lp);
        Type *i8PtrType = b.getInt8PtrTy();

        // get the personality func
        Value *personality =llvmBuilder.exceptionPersonalityFunc;

        // create a catch selector or a cleanup selector
        Value *exStruct;
        if (cdata) {
            // We're in a try/catch.  create the incomplete selector function
            // call (class impls will get filled in later)
            IncompleteCatchSelector *sel  =
                new IncompleteCatchSelector(personality, b.GetInsertBlock());
            cdata->selectors.push_back(sel);
            exStruct = sel;
        } else {
            // cleanups only
            LandingPadInst *lpi =
                b.CreateLandingPad(exStructType, personality, 0);
            lpi->setCleanup(true);
            exStruct = lpi;
        }

        // get the function-wide exception structure
        BMemVarDefImpl *exStructImpl =
            BMemVarDefImplPtr::rcast(exStructVar->impl);
        b.CreateStore(exStruct, exStructImpl->getRep(llvmBuilder));

        b.CreateBr(next);
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
