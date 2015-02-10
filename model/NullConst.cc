// Copyright 2009-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "NullConst.h"

#include "builder/Builder.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

ResultExprPtr NullConst::emit(Context &context) {
    return context.builder.emitNull(context, this);
}

ExprPtr NullConst::convert(Context &context, TypeDef *newType) {
    if (newType->matches(*context.construct->voidType))
        return 0;
    return new NullConst(newType);
}

void NullConst::writeTo(ostream &out) const {
    out << "null";
}

bool NullConst::isProductive() const {
    return false;
}
