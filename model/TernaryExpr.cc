// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "TernaryExpr.h"

#include "builder/Builder.h"
#include "Context.h"
#include "ResultExpr.h"
#include "TypeDef.h"

using namespace std;
using namespace model;

ResultExprPtr TernaryExpr::emit(Context &context) {
    return context.builder.emitTernary(context, this);
}

void TernaryExpr::writeTo(ostream &out) const {
    out << "(";
    cond->writeTo(out);
    out << " ? ";
    trueVal->writeTo(out);
    out << " : ";
    if (falseVal)
        falseVal->writeTo(out);
    else
        out << "void";
    out << ")";
}

bool TernaryExpr::isProductive() const {
    return trueVal->isProductive() || falseVal && falseVal->isProductive();
}