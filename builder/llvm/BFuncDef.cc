// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFuncDef.h"
#include "model/Context.h"
#include "builder/llvm/LLVMBuilder.h"

#include <string>

#include <llvm/Function.h>

using namespace builder::mvll;

void BFuncDef::setOwner(model::Namespace *o) {
    owner = o;
    fullName.clear();    
    // if an overridden symbolName isn't set, we use the canonical name
    if (symbolName.empty())
        rep->setName(getFullName());
}

llvm::Function * BFuncDef::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModFunc(this);
    return rep;
}


// only used for annotation functions
void *BFuncDef::getFuncAddr(Builder &builder) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder&>(builder);
    return b.getFuncAddr(getRep(b));
}
