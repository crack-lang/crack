// Copyright 2009 Google Inc.

#include "NullConst.h"

#include "builder/Builder.h"
#include "Context.h"
#include "ResultExpr.h"

using namespace model;
using namespace std;

ResultExprPtr NullConst::emit(Context &context) {
    return context.builder.emitNull(context, this);
}

ExprPtr NullConst::convert(Context &context, TypeDef *newType) {
    return new NullConst(newType);
}

void NullConst::writeTo(ostream &out) const {
    out << "null";
}
