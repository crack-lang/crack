// Copyright 2014 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Deref_h_
#define _model_Deref_h_

#include "Expr.h"

namespace model {

class Context;
SPUG_RCPTR(VarDef);

SPUG_RCPTR(Deref);

/**
 * The "Deref" class is a transient model object used for binding a receiver
 * with the definition that is being applied to it (as in the case of a method
 * call or a field reference or assignment).  It can not be emitted, and must
 * be replaced when the actual action to be performed is determined.
 *
 * The type of a Deref is the type of its definition.
 */
class Deref : public Expr {
    protected:
        /**
         * For use by AttrDeref, where we defer initialization of Expr's
         * "type".
         */
        Deref(Expr *receiver);

        /**
         * Returns the dereferencing expression.
         */
        virtual ExprPtr createDeref(Context &context) const;

    public:
        ExprPtr receiver;
        VarDefPtr def;
        bool squashVirtual;

        Deref(Expr *receiver, VarDef *def);

        virtual ResultExprPtr emit(Context &context);
        virtual bool isProductive() const { return false; }
        virtual bool isVolatile() const { return true; }
        virtual void writeTo(std::ostream &out) const;
        virtual ExprPtr makeCall(Context &context,
                                 std::vector<ExprPtr> &args
                                 ) const;

        /**
         * Convert the dereference to an assignment.
         */
        virtual ExprPtr makeAssignment(Context &context, Expr *val);
};

}

#endif
