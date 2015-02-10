// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Expr.h"

#include "spug/check.h"
#include "spug/StringFmt.h"
#include "builder/Builder.h"
#include "Context.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

Expr::Expr(TypeDef *type) : type(type) {}

Expr::~Expr() {}

string Expr::getTypeDisplayName() const {
    if (type)
        return type->getDisplayName();
    else
        return "<Overload>";
}

void Expr::emitCond(Context &context) {
    context.builder.emitTest(context, this);
}

ExprPtr Expr::convert(Context &context, TypeDef *newType) {
    
    // see if we're already of the right type
    if (newType->matches(*type))
        return this;
    
    // see if there's a converter
    FuncDefPtr converter = type->getConverter(context, *newType);
    if (converter) {
        FuncCallPtr convCall =
           context.builder.createFuncCall(converter.get());
        convCall->receiver = this;
        return convCall;
    }
    
    // can't convert
    return 0;
}

bool Expr::isProductive() const {
    // by default, all expresssions returning pointer types are productive
    return type->pointer;
}

bool Expr::isAdaptive() const {
    return false;
}

ExprPtr Expr::foldConstants() { return this; }

ExprPtr Expr::makeCall(Context &context, FuncCall::ExprVec &args) const {
    FuncDefPtr func = context.lookUp("oper call", args, type.get());
    if (!func)
        context.error(SPUG_FSTR("Instance of " << type->getDisplayName() <<
                                 " does not have an 'oper call' method "
                                 " and can not be called."
                                )
                      );
    SPUG_CHECK(!func->isExplicitlyScoped(),
               "Unexpected use of an explicitly scoped function in a "
                "general expression."
               );
    FuncCallPtr funcCall = context.builder.createFuncCall(func.get());
    funcCall->args = args;
    if (func->needsReceiver()) {
        SPUG_CHECK(type->isDerivedFrom(func->receiverType.get()),
                   "Receiver in an 'oper call' is not the type that it "
                    "was obtained from.  receiver type = " <<
                    func->receiverType->getDisplayName() <<
                    " expr type = " << type->getDisplayName()
                   );
        funcCall->receiver = const_cast<Expr *>(this);
    }
    return funcCall;
}

void Expr::dump() const { writeTo(std::cerr); }