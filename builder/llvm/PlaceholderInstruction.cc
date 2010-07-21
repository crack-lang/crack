// Copyright 2010 Google Inc.

#include "PlaceholderInstruction.h"

using namespace llvm;
using namespace builder::mvll;

void PlaceholderInstruction::fix() {
    IRBuilder<> builder(getParent(), this);
    insertInstructions(builder);
    this->eraseFromParent();
    // ADD NO CODE AFTER SELF-DELETION.
}
