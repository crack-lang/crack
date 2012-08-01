// Copyright 2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "ConstSequenceExpr.h"

#include "CleanupFrame.h"
#include "Context.h"
#include "FuncCall.h"
#include "ResultExpr.h"

using namespace model;

ResultExprPtr ConstSequenceExpr::emit(Context &context) {
    ResultExprPtr containerResult = container->emit(context);
    
    // emit all of the element append epxressions
    for (int i = 0; i < elems.size(); ++i) {
        context.createCleanupFrame();
        
        // this is rather ugly.  We have to set the receiver here to avoid 
        // having to either create a variable to hold the container or 
        // re-evalutating the container expression every time (effectively 
        // allocating one container for each element)
        elems[i]->receiver = containerResult;
        elems[i]->emit(context)->handleTransient(context);
        context.closeCleanupFrame();
    }

    // re-emit the result.
    containerResult->emit(context);
    return containerResult;
}
    
void ConstSequenceExpr::writeTo(std::ostream &out) const {
    out << "[";
    for (int i = 0; i < elems.size(); ++i) {
        elems[i]->writeTo(out);
        out << ", ";
    }
    out << "]";
}
