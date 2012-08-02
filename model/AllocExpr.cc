// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
