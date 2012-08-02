// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "PlaceholderInstruction.h"

using namespace llvm;
using namespace builder::mvll;

void PlaceholderInstruction::fix() {
    IRBuilder<> builder(getParent(), this);
    insertInstructions(builder);
    this->eraseFromParent();
    // ADD NO CODE AFTER SELF-DELETION.
}
