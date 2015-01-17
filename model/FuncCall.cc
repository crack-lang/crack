// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "FuncCall.h"

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
    return context.builder.emitFuncCall(context, this);
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
        out << args[i]->getTypeDisplayName();
    }

    return out;
}
