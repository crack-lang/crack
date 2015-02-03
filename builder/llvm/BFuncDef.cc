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

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace builder::mvll;
using namespace llvm;
using namespace std;

void BFuncDef::setOwner(model::Namespace *o) {
    owner = o;
    fullName.clear();    
    // if an overridden symbolName isn't set, we use the canonical name
    if (symbolName.empty())
        rep->setName(getUniqueId());
}

Constant *BFuncDef::getRep(LLVMBuilder &builder) {
    
    // Load the function rep for the correct module.
    if (repModuleId != builder.moduleDef->repId) {
        repModuleId = builder.moduleDef->repId;
        if (flags & FuncDef::abstract)
            rep = Constant::getNullValue(llvmFuncType->getPointerTo());
        else
            rep = builder.getModFunc(this);
        return rep;
    } else {
        return rep;
    }
}

Function *BFuncDef::getFuncRep(LLVMBuilder &builder) {
    return dyn_cast<Function>(getRep(builder));
}

void BFuncDef::setRep(llvm::Constant *newRep, int newRepModuleId) {
    SPUG_CHECK(!llvmFuncType, 
               "setRep called on function " << getFullName() <<
                " which already has a type defined."
               );
    rep = newRep;
    repModuleId = newRepModuleId;
    llvmFuncType = dyn_cast<FunctionType>(
        dyn_cast<PointerType>(rep->getType())->getElementType()
    );
    SPUG_CHECK(llvmFuncType, 
               "BFuncDef::setRep() called with a non-function for " << 
                getFullName()
               );
}
    
void BFuncDef::fixModule(Module *oldMod, Module *newMod) {
    Function *func = llvm::dyn_cast<Function>(rep);
    if (func && func->getParent() == oldMod)
        rep = newMod->getFunction(func->getName());
}


// only used for annotation functions
void *BFuncDef::getFuncAddr(Builder &builder) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder&>(builder);
    return b.getFuncAddr(getFuncRep(b));
}
