// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BFieldRef_h_
#define _builder_llvm_BFieldRef_h_

#include "model/VarRef.h"
#include "model/Expr.h"
#include "model/Context.h"
#include "model/ResultExpr.h"

namespace model {
    class VarDef;
}

namespace builder {
namespace mvll {

class BFieldRef : public model::VarRef {
public:
    model::ExprPtr aggregate;

    BFieldRef(model::Expr *aggregate, model::VarDef *varDef) :
            aggregate(aggregate),
            VarRef(varDef) {
    }

    model::ResultExprPtr emit(model::Context &context);

};

} // end namespace builder::vmll
} // end namespace builder

#endif
