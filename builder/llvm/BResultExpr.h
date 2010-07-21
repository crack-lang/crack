// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

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
