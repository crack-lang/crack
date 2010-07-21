// Copyright 2010 Google Inc.

#ifndef _builder_llvm_PlaceholderInstruction_h_
#define _builder_llvm_PlaceholderInstruction_h_

#include <llvm/Instruction.h>
#include <llvm/Support/IRBuilder.h>

namespace builder { namespace mvll {

/**
 * This is a special instruction that serves as a placeholder for 
 * operations where we dereference incomplete types.  These get stored in 
 * a block and subsequently replaced with a reference to the actual type.
 */    
class PlaceholderInstruction : public llvm::Instruction {
    public:
        PlaceholderInstruction(const llvm::Type *type,
                               llvm::BasicBlock *parent,
                               llvm::Use *ops = 0, 
                               unsigned opCount = 0
                               ) :
            Instruction(type, OtherOpsEnd + 1, ops, opCount, parent) {
        }

        PlaceholderInstruction(const llvm::Type *type,
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
