// Copyright 2011 Google Inc.

#include "GetRegisterExpr.h"

#include "Context.h"
#include "ResultExpr.h"

using namespace model;

ResultExprPtr GetRegisterExpr::emit(Context &context) {
    return context.reg->emit(context);
}

void GetRegisterExpr::writeTo(std::ostream &out) const {
    out << "register";
}

bool GetRegisterExpr::isProductive() const {
    // like result expressions, register expressions are inherently 
    // non-productive.
    return false;
}