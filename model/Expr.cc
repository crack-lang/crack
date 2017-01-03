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
#include "GetRegisterExpr.h"
#include "MultiExpr.h"
#include "SetRegisterExpr.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

Expr::Expr(TypeDef *type) : type(type) {}

Expr::~Expr() {}

string Expr::getTypeDisplayName() const {
    return type->getDisplayName();
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

bool Expr::isVolatile() const {
    return false;
}

namespace {
    /**
     * Converts a volatile expression to a non-volatile one.
     */
    class NonVolatileExprAdapter : public Expr {
        private:
            ExprPtr sourceExpr;

        public:
            NonVolatileExprAdapter(Expr *sourceExpr) :
                Expr(sourceExpr->type.get()),
                sourceExpr(sourceExpr) {
            }

            virtual ResultExprPtr emit(Context &context) {
                ResultExprPtr result = sourceExpr->emit(context);
                result->sourceExpr = this;
                return result;
            }

            virtual bool isProductive() const {
                return true;
            }
            
            virtual bool isVolatile() const {
                return true;
            }

            virtual void writeTo(std::ostream &out) const {
                out << "productive(" << *sourceExpr << ')';
            }
    };
}

ExprPtr Expr::makeNonVolatile(Context &context) {
    // We don't need this if the expression is already productive or
    // non-volatile.
    if (isProductive() || !isVolatile())
        return this;

    MultiExprPtr seq = new MultiExpr();
    GetRegisterExprPtr getRegExpr = new GetRegisterExpr(type.get());
    SetRegisterExprPtr setRegExpr =
        new SetRegisterExpr(getRegExpr.get(), new NonVolatileExprAdapter(this));
    seq->add(setRegExpr.get());

    // If there's no "oper bind", just return this.
    // XXX be nice to check this before allocating all that stuff.
    ExprPtr bindCall = getRegExpr->makeOperBind(context);
    if (!bindCall)
        return this;

    seq->add(bindCall.get());
    seq->add(getRegExpr.get());
    seq->type = getRegExpr->type;
//    cerr << "Converted to productive: " << *seq << endl;
    return seq;
}

ExprPtr Expr::makeOperBind(Context &context) {
    FuncCall::ExprVec args;
    FuncDefPtr bindFunc = context.lookUpNoArgs("oper bind", false, type.get());
    if (!bindFunc) {
        if (!type->noBindInferred) type->noBindInferred = context.getLocation();
        return 0;
    }

    // got a bind function: create a bind call and emit it.  (emit should
    // return a ResultExpr for a void object, so we don't need to do anything
    // special for it).
    FuncCallPtr bindCall = context.builder.createFuncCall(bindFunc.get());
    bindCall->receiver = this;
    return bindCall;
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