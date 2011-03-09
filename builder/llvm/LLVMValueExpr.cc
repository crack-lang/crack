// Copyright 2011 Google Inc

#include "LLVMValueExpr.h"

#include "model/Context.h"
#include "BResultExpr.h"
#include "LLVMBuilder.h"

using namespace model;
using namespace builder::mvll;

ResultExprPtr LLVMValueExpr::emit(Context &context) {
    LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue = value;
    return new BResultExpr(this, value);
}

void LLVMValueExpr::writeTo(std::ostream &out) const {
    out << "LLVMValueExpr(" << value << ")";
}

bool LLVMValueExpr::isProductive() const {
    return false;
}

