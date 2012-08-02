// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "MultiExpr.h"

#include "Branchpoint.h"
#include "CleanupFrame.h"
#include "Context.h"
#include "FuncCall.h"
#include "ResultExpr.h"
#include "builder/Builder.h"

using namespace model;

ResultExprPtr MultiExpr::emit(Context &context) {
    // must be at least one element    
    assert(elems.size());

    // emit all but the last of formatter functions
    int i;
    for (i = 0; i < elems.size() - 1; ++i)
        elems[i]->emit(context)->handleTransient(context);

    return elems[i]->emit(context);
}
    
void MultiExpr::writeTo(std::ostream &out) const {
    out << "(";
    for (int i = 0; i < elems.size(); ++i) {
        elems[i]->writeTo(out);
        out << ", ";
    }
    out << ")";
}

bool MultiExpr::isProductive() const {
    return elems.back()->isProductive();
}
