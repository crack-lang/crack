// Copyright 2009-2010 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "VarRef.h"

#include "spug/check.h"
#include "spug/StringFmt.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;
using namespace std;

VarRef::VarRef(VarDef *def) :
    Expr(def->type.get()),
    def(def) {
}

TypeDefPtr VarRef::getType(Context &context) const {
    if (OverloadDef *ovld = OverloadDefPtr::rcast(def)) {
        FuncDefPtr func = ovld->getSingleFunction();
        if (!func)
            context.error(
                SPUG_FSTR("Invalid use of ambiguous overload " <<
                           ovld->getDisplayName() <<
                           "."
                          )
            );
        return func->type;
    } else {
        return type;
    }
}

ResultExprPtr VarRef::emit(Context &context) {
    if (OverloadDefPtr::rcast(def) && !def->impl)
        context.error(SPUG_FSTR("Cannot obtain value of builtin function " <<
                                 def->getDisplayName()
                                )
                      );
    assert(def->impl);
    return def->impl->emitRef(context, this);
}

ExprPtr VarRef::convert(Context &context, TypeDef *type) {
    if (OverloadDef *ovld = OverloadDefPtr::rcast(def)) {
        FuncDef *func = ovld->getMatch(type);
        if (func) {
            return context.createVarRef(func);
        } else if (FuncDefPtr funcDef = ovld->getSingleFunction()) {
            VarRefPtr varRef = context.createVarRef(funcDef.get());
            return varRef->convert(context, type);
        }
    } else {
        return Expr::convert(context, type);
    }
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
    funcCall->args = args;
    if (func->needsReceiver()) {

        // If this is an "oper call" method use the var as the receiver.
        if (func->name == "oper call") {
            SPUG_CHECK(type->isDerivedFrom(func->receiverType.get()),
                       "Receiver in an 'oper call' is not the type that it "
                        "was obtained from.  receiver type = " <<
                        func->receiverType->getDisplayName() <<
                        " var type = " << type->getDisplayName()
                       );
            funcCall->receiver = const_cast<VarRef *>(this);
        } else {
            funcCall->receiver = context.makeThisRef(def->name);

            TypeDefPtr owner = TypeDefPtr::cast(func->getOwner());
            if (!funcCall->receiver->type->isDerivedFrom(owner.get()))
                context.error(
                    SPUG_FSTR("'this' variable is not an instance of " <<
                               owner->name << " for call to method " <<
                               func->name
                              )
                );
        }
    }

    // If it's an explicitly scoped function, make sure it's not abstract.
    if (def->isExplicitlyScoped() && (func->flags & FuncDef::abstract))
        context.error(
            SPUG_FSTR("Abstract function " << func->name << "(" <<
                       args << ") can not be called."
                      )
        );

    return funcCall;
}
