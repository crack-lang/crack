// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_ResultExpr_h_
#define _model_ResultExpr_h_

#include "Expr.h"

namespace model {

SPUG_RCPTR(ResultExpr);

/**
 * A result expression is the stored result of a previous expression used for 
 * cleanup.
 * 
 * This is an abstract class: the builder should produce a concrete derived 
 * class implementing the "emit()" method.
 */
class ResultExpr : public Expr {
    public:
        ExprPtr sourceExpr;

        ResultExpr(Expr *sourceExpr) : 
            Expr(sourceExpr->type.get()), 
            sourceExpr(sourceExpr) {
        }

        /**
         * Does garbage collection processing for an assignment: if the original 
         * expression was productive, does nothing (the variable being 
         * assigned will consume the new reference).  If not, generates a 
         * "bind" operation to cause the expression to be owned by the 
         * variable.
         */        
        void handleAssignment(Context &context);
        
        /**
         * Handles the cleanup of a transient reference.  If the source 
         * expression is productive, adds it to the cleanups.
         */
        void handleTransient(Context &context);
        
        /**
         * Forces the expression to be added to cleanups, whether it is 
         * productive or not (but not if it has no 'oper release').
         */
        void forceCleanup(Context &context);
        
        /** Overrides isProductive() to delegate to the source expr. */
        virtual bool isProductive() const;

        virtual void writeTo(std::ostream &out) const;
};

} // namespace model

#endif
