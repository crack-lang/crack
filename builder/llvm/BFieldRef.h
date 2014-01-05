// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_BFieldRef_h_
#define _builder_llvm_BFieldRef_h_

#include "model/VarRef.h"
#include "model/Expr.h"
#include "model/Context.h"
#include "model/ResultExpr.h"

namespace llvm {
    class Value;
}

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

    llvm::Value *emitAddr(model::Context &context) const;

};

} // end namespace builder::vmll
} // end namespace builder

#endif
