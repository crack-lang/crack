// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "BFuncDef.h"
#include "model/Context.h"
#include "builder/llvm/LLVMBuilder.h"

#include <string>

#include <llvm/Function.h>

using namespace builder::mvll;
using namespace llvm;

void BFuncDef::setOwner(model::Namespace *o) {
    owner = o;
    fullName.clear();    
    // if an overridden symbolName isn't set, we use the canonical name
    if (symbolName.empty())
        rep->setName(getUniqueId());
}

Constant *BFuncDef::getRep(LLVMBuilder &builder) {
    
    // load the function for the correct module, but not for abstract methods 
    // (that would result in an "extern" method for an abstract method, which 
    // is an unresolved external)
    Function *funcRep = dyn_cast<Function>(rep);
    if (funcRep && funcRep->getParent() != builder.module)
        return builder.getModFunc(this, funcRep);
    else
        return rep;
}

Function *BFuncDef::getFuncRep(LLVMBuilder &builder) {
    Function *result = dyn_cast<Function>(getRep(builder));
    SPUG_CHECK(result, 
               "Attempted to call BFuncDef::getFuncRep() on function " <<
                getDisplayName() << " which is not a function."
               );
    return result;
}


// only used for annotation functions
void *BFuncDef::getFuncAddr(Builder &builder) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder&>(builder);
    return b.getFuncAddr(getFuncRep(b));
}
