// Copyright 2011 Google Inc.

#include "BBuilderContextData.h"

#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/IRBuilder.h>
#include "model/Expr.h"

using namespace std;
using namespace llvm;
using namespace builder::mvll;

BasicBlock *BBuilderContextData::getUnwindBlock(Function *func) {
    if (!unwindBlock) {
        unwindBlock = BasicBlock::Create(getGlobalContext(), "unwind", func);
        IRBuilder<> b(unwindBlock);
        b.CreateUnwind();
    }

    // assertion to make sure this is the right unwind block
    if (unwindBlock->getParent() != func) {
        string bdataFunc = unwindBlock->getParent()->getName();
        string curFunc = func->getName();
        cerr << "bad function for unwind block, got " <<
            bdataFunc << " expected " << curFunc << endl;
        assert(false);
    }

    return unwindBlock;
}
