// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_BBuilderContextData_h_
#define _builder_llvm_BBuilderContextData_h_

#include <vector>
#include "model/BuilderContextData.h"
#include "model/Context.h"
#include <spug/RCPtr.h>
#include <llvm/IR/IRBuilder.h>

namespace llvm {
    class Function;
    class BasicBlock;
    class SwitchInst;
    class Value;
}

namespace builder { namespace mvll {

class IncompleteCatchSelector;

SPUG_RCPTR(BBuilderContextData);
class BModuleDef;
SPUG_RCPTR(BTypeDef);

class BBuilderContextData : public model::BuilderContextData {
public:

    // stores the type to catch and the block to branch to if the type is 
    // caught.
    struct CatchBranch {
        BTypeDefPtr type;
        llvm::BasicBlock *block;
        
        CatchBranch(BTypeDef *type, llvm::BasicBlock *block) :
            type(type),
            block(block) {
        }
    };

    // state information on the current try/catch block.
    SPUG_RCPTR(CatchData);
    struct CatchData : spug::RCBase {
        // all of the type/branch combinations in the catch clauses (this will 
        // be augmented with all of the combos for statements that this is 
        // nested in)
        std::vector<CatchBranch> catches;
        
        // list of the incomplete catch selectors for the block.
        std::vector<IncompleteCatchSelector *> selectors;
        
        // the switch instruction for the catch blocks
        llvm::SwitchInst *switchInst;
        
        // catch data for nested catches.
        std::vector<CatchDataPtr> nested;
        
        // true if some of the blocks in the try statement are non-terminal
        bool nonTerminal;
        
        CatchData() : nonTerminal(false) {}

        /**
         * Populate the 'values' array with the implementations of the types 
         * in 'types'.
         */
        void populateClassImpls(std::vector<llvm::Value *> &values,
                                BModuleDef *module
                                );

        /** 
         * Fix all of the selectors in the context by filling in the class 
         * implementation objects and calling Incomplete::fix() on them to 
         * convert them to calls to llvm.eh.selector().
         */
        void fixAllSelectors(BModuleDef *module);
    };

    model::Context *context;
    llvm::Function *func;
    llvm::BasicBlock *block,
        *unwindBlock,      // the unwind block for the catch/function
        *nextCleanupBlock; // the current next cleanup block

    BBuilderContextData(model::Context *context) :
            context(context),
            func(0),
            block(0),
            unwindBlock(0),
            nextCleanupBlock(0) {
    }
    
    static BBuilderContextData *get(model::Context *context) {
        if (!context->builderData)
            context->builderData = new BBuilderContextData(context);
        return BBuilderContextDataPtr::rcast(context->builderData);
    }
    
    CatchDataPtr getCatchData() {
        if (!catchData)
            catchData = new CatchData();
        return catchData;
    }
    
    void deleteCatchData() {
        assert(catchData);
        catchData = 0;
    }

    llvm::Value *getExceptionLandingPadResult(llvm::IRBuilder<> &builder);
    llvm::BasicBlock *getUnwindBlock(llvm::Function *func);

private:
    CatchDataPtr catchData;
    BBuilderContextData(const BBuilderContextData &);
};

}} // end namespace builder::vmll

#endif
