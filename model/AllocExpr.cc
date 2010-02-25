// Copyright 2009 Google Inc.

#include "AllocExpr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "TypeDef.h"
#include "ResultExpr.h"

using namespace model;
using namespace std;

ResultExprPtr AllocExpr::emit(Context &context) {
    return context.builder.emitAlloc(context, this);
}

void AllocExpr::writeTo(ostream &out) const {
    out << "alloc(" << type->name << ")";
}
