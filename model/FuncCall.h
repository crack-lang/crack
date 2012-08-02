// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_FuncCall_h_
#define _model_FuncCall_h_

#include <string>
#include <vector>
#include "Expr.h"

namespace model {

SPUG_RCPTR(FuncDef);

SPUG_RCPTR(FuncCall);

class FuncCall : public Expr {
    public:
        FuncDefPtr func;
        typedef std::vector<ExprPtr> ExprVec;
        ExprVec args;
        
        // if FuncCall is a method, this is the receiver (the "this"), 
        // otherwise it will be null
        ExprPtr receiver;
        
        // if true, the function call is virtual (func->flags must include 
        // "virtualized" in this case)
        bool virtualized;
        
        /**
         * @param squashVirtual If true, call a virtualized function 
         * directly, without the use of the vtable (causes virtualized to be 
         * set to false, regardless of whether funcDef is virtual).
         */
        FuncCall(FuncDef *funcDef, bool squashVirtual = false);

        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};

std::ostream &operator <<(std::ostream &out, const FuncCall::ExprVec &args);

} // namespace model

#endif
