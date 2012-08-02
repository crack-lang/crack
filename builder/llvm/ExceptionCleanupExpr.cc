// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "ExceptionCleanupExpr.h"

#include "model/Context.h"
#include "model/ResultExpr.h"
#include "LLVMBuilder.h"

using namespace model;
using namespace builder::mvll;

ResultExprPtr ExceptionCleanupExpr::emit(model::Context &context) {
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.emitExceptionCleanupExpr(context);
    
    // we can get away with this because the results of cleanup expressions 
    // are ignored.
    return 0;
}

void ExceptionCleanupExpr::writeTo(std::ostream &out) const {
    out << "LLVMExceptionCleanupExpr";
}
        
bool ExceptionCleanupExpr::isProductive() const { return false; }
