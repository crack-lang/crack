// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFuncDef.h"
#include "model/Context.h"

#include <llvm/Function.h>

namespace builder {
namespace mvll {

llvm::Function * BFuncDef::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModFunc(this);
    return rep;
}


} // end namespace builder::vmll
} // end namespace builder

