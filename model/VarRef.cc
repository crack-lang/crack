// Copyright 2009-2010 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "VarRef.h"

#include "spug/StringFmt.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;
using namespace std;

VarRef::VarRef(VarDef *def) :
    Expr(def->type.get()),
    def(def) {
}

ResultExprPtr VarRef::emit(Context &context) {
    assert(def->impl);
    return def->impl->emitRef(context, this);
}

bool VarRef::isProductive() const {
    return false;
}

void VarRef::writeTo(ostream &out) const {
    out << "ref(" << def->name << ')';
}

ExprPtr VarRef::makeCall(Context &context,
                         std::vector<ExprPtr> &args
                         ) const {
    FuncDefPtr func = def->getFuncDef(context, args);
    FuncCallPtr funcCall = context.builder.createFuncCall(
        func.get(),
        def->isExplicitlyScoped()
    );
    // xxx squash virtual
    //      this is tricky.  I think in order to do this we need some kind of
    //      "wrapper def" that gets created when a name is explicitly scoped.

    funcCall->args = args;
    if (func->needsReceiver()) {
        funcCall->receiver = context.makeThisRef(def->name);

        TypeDefPtr owner = TypeDefPtr::cast(func->getOwner());
        if (!funcCall->receiver->type->isDerivedFrom(owner.get()))
            context.error(SPUG_FSTR("'this' is not an instance of " <<
                                     owner->name
                                    )
                          );
    }
    return funcCall;
}
