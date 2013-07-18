// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_PlaceholderInstruction_h_
#define _builder_llvm_PlaceholderInstruction_h_

#include <llvm/IR/Instruction.h>
#include <llvm/IR/IRBuilder.h>

namespace builder { namespace mvll {

/**
 * This is a special instruction that serves as a placeholder for 
 * operations where we dereference incomplete types.  These get stored in 
 * a block and subsequently replaced with a reference to the actual type.
 */    
class PlaceholderInstruction : public llvm::Instruction {
    public:
        PlaceholderInstruction(llvm::Type *type,
                               llvm::BasicBlock *parent,
                               llvm::Use *ops = 0, 
                               unsigned opCount = 0
                               ) :
            Instruction(type, OtherOpsEnd + 1, ops, opCount, parent) {
        }

        PlaceholderInstruction(llvm::Type *type,
                               llvm::Instruction *insertBefore = 0,
                               llvm::Use *ops = 0,
                               unsigned opCount = 0
                               ) :
            Instruction(type, OtherOpsEnd + 1, ops, opCount, 
                        insertBefore
                        ) {
        }
        
        /** Replace the placeholder with a real instruction. */
        void fix();
        
        virtual void insertInstructions(llvm::IRBuilder<> &builder) = 0;
};

}} // namespace builder::mvll

#endif
