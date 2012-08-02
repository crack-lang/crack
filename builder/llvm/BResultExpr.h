// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_BResultExpr_h_
#define _builder_llvm_BResultExpr_h_

#include "LLVMBuilder.h"
#include "model/Context.h"
#include "model/ResultExpr.h"

namespace llvm {
    class Value;
}

namespace builder {
namespace mvll {

class BResultExpr : public model::ResultExpr {
    public:
        llvm::Value *value;

        BResultExpr(model::Expr *sourceExpr, llvm::Value *value) :
            ResultExpr(sourceExpr),
            value(value) {
        }
            
        model::ResultExprPtr emit(model::Context &context) {
            dynamic_cast<LLVMBuilder &>(context.builder).lastValue =
                value;
            return new BResultExpr(this, value);
        }
};
    

} // end namespace builder::vmll
} // end namespace builder

#endif
