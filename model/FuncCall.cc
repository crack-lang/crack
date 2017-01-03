// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "FuncCall.h"

#include "spug/stlutil.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;

FuncCall::FuncCall(FuncDef *funcDef, bool squashVirtual) :
    Expr(funcDef->returnType.get()),
    func(funcDef),
    virtualized(squashVirtual ? false : 
                                (funcDef->flags & FuncDef::virtualized)
                ) {
    if (!funcDef->returnType)
        std::cerr << "bad func def: " << funcDef->name << std::endl;
    assert(funcDef->returnType);
}

ResultExprPtr FuncCall::emit(Context &context) {
    ResultExprPtr result;
    if (func->name != "oper bind" && func->name != "oper release" &&
        func->name != "oper init" && func->name != "oper del" &&
        func->name.compare(0, 10, "oper from ")
        ) {
        ExprVec productiveArgs;
        ExprPtr productiveReceiver;
        SPUG_FOR(ExprVec, iter, args) {
            ExprPtr prod;
            productiveArgs.push_back(prod = (*iter)->makeNonVolatile(context));
        }
        args = productiveArgs;
        if (receiver)
            receiver = receiver->makeNonVolatile(context);

        result =
            context.builder.emitFuncCall(context, this);
    } else {
        result = context.builder.emitFuncCall(context, this);
    }

    return result;
}

void FuncCall::writeTo(std::ostream &out) const {
    if (receiver)
        out << "(" << *receiver << ").";
    out << "call \"" << func->name << "\"(";
    for (ExprVec::const_iterator iter = args.begin();
         iter != args.end();
         ++iter
         )
        out << **iter << ", ";
    out << ")";
}

std::ostream &model::operator <<(std::ostream &out, 
                                 const FuncCall::ExprVec &args
                                 ) {
    for (int i = 0; i < args.size(); i++) {
        if (i > 0) out << ", ";
        if (!args[i]->type)
            out << "NULL-TYPE(" << *args[i] << ")" << std::endl;
        else
            out << args[i]->getTypeDisplayName();
    }

    return out;
}
