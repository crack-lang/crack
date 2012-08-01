// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "GetRegisterExpr.h"

#include "Context.h"
#include "ResultExpr.h"

using namespace model;

ResultExprPtr GetRegisterExpr::emit(Context &context) {
    return val->emit(context);
}

void GetRegisterExpr::writeTo(std::ostream &out) const {
    out << "register";
}

bool GetRegisterExpr::isProductive() const {
    // like result expressions, register expressions are inherently
    // non-productive.
    return false;
}