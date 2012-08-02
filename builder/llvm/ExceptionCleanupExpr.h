// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_ExceptionCleanupExpr_h_
#define _builder_llvm_ExceptionCleanupExpr_h_

#include "model/Expr.h"

namespace builder { namespace mvll {

/**
 * Expression class for wrapping LLVMValues.
 */
class ExceptionCleanupExpr : public model::Expr {
    public:

        ExceptionCleanupExpr(model::TypeDef *type) : Expr(type) {}

        virtual model::ResultExprPtr emit(model::Context &context);
        virtual void writeTo(std::ostream &out) const;
        
        /** Override isProductive(), always false. */
        virtual bool isProductive() const;
};
    
}} // namespace builder::llvm

#endif
