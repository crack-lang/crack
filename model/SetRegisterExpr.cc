// Copyright 2011 Google Inc.

#include "SetRegisterExpr.h"

#include "Context.h"
#include "ResultExpr.h"

using namespace model;

ResultExprPtr SetRegisterExpr::emit(Context &context) {
    ResultExprPtr result = expr->emit(context);
    reg->val = result;
    return result;
}

void SetRegisterExpr::writeTo(std::ostream &out) const {
    out << "register = ";
    expr->writeTo(out);
}

bool SetRegisterExpr::isProductive() const {
    // delegate this to the expression we wrap
    return expr->isProductive();
}
