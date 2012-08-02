// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "StrConst.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

StrConst::StrConst(TypeDef *type, const std::string &val) :
    Expr(type),
    val(val) {
}

ResultExprPtr StrConst::emit(Context &context) { 
    return context.builder.emitStrConst(context, this);
}

void StrConst::writeTo(ostream &out) const {
    out << '"' << val << '"';
}
