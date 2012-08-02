// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
