// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFuncDef.h"
#include "model/Context.h"

#include <string>

#include <llvm/Function.h>

using namespace builder::mvll;

void BFuncDef::setOwner(model::Namespace *o) {
    owner = o;
    // XXX hack
    std::string fn(getFullName());
    if (rep->getNameStr() != "calloc")
        rep->setName(fn);
}

llvm::Function * BFuncDef::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModFunc(this);
    return rep;
}


