// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFuncDef.h"
#include "model/Context.h"

#include <string>

#include <llvm/Function.h>

using namespace builder::mvll;

void BFuncDef::setOwner(model::Namespace *o) {
    owner = o;
    fullName.clear();    
    if (!(flags & external))
        rep->setName(getFullName());
}

llvm::Function * BFuncDef::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModFunc(this);
    return rep;
}


void *BFuncDef::getFuncAddr(Builder &builder) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder&>(builder);
    return b.getFuncAddr(getRep(b));
}