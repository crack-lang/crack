// Copyright 2020 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_SafeNavExpr_h_
#define _model_SafeNavExpr_h_

#include "TernaryExpr.h"

namespace model {

/**
 * The "safe navigation" operator.  This is basically a ternary operator with
 * a special feature:  when converted to a function call, absorbs the
 * function call into the non-null condition.
 */
class SafeNavExpr : public TernaryExpr {
    public:
        /**
         * Note that for non-void expressions (that is, cases where the type
         * of the non-null condition is not void), we can use the non-null
         * case as 'trueVal', so we can just use "primary is null" as the
         * condition without having to invert it.
         *
         * For void expressions, 'falseVal' should be set to null (because we
         * can't create a NullConst of type void), so we have to invert the
         * condition and make 'trueVal' the non-null case.
         */
        SafeNavExpr(Expr *primary, Expr *trueVal, Expr *falseVal) :
            // Use the type of the first expression here, for void expressions
            // the second case isn't emitted.
            TernaryExpr(primary, trueVal, falseVal, trueVal->type.get()) {
        }

        virtual void writeTo(std::ostream &out) const;
        virtual ExprPtr makeCall(Context &context,
                                 std::vector<ExprPtr> &args
                                 ) const;
};

}

#endif