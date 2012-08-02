// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_AssignExpr_h_
#define _model_AssignExpr_h_

#include "Expr.h"

namespace parser {
    class Token;
};

namespace model {

class Context;
SPUG_RCPTR(Expr);
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDef);

SPUG_RCPTR(AssignExpr);

/**
 * An assignment expression.
 */
class AssignExpr : public Expr {
    public:
        ExprPtr aggregate, value;
        VarDefPtr var;

        AssignExpr(Expr *aggregate, VarDef *var, Expr *value);

        /**
         * Create a new AssignExpr, check for errors.
         * @param aggregate the aggregate that the variable is a member of
         * @param var the variable
         * @param value the value to be assigned to the variable.
         */
        static AssignExprPtr create(Context &context,
                                    Expr *aggregate,
                                    VarDef *var, 
                                    Expr *value
                                    );

        /**
         * Create a new AssignExpr, check for errors.  Just like the other 
         * create() method only for non-instance variables (statics, globals 
         * and locals)
         * 
         * @param var the variable
         * @param value the value to be assigned to the variable.
         */
        static AssignExprPtr create(Context &context,
                                    VarDef *var, 
                                    Expr *value
                                    ) {
            return create(context, 0, var, value);
        }

        /** Emit the expression in the given context. */
        virtual ResultExprPtr emit(Context &context);

        // overrides Expr, assignments are non-productive.        
        virtual bool isProductive() const;
        virtual void writeTo(std::ostream &out) const;
};

} // namespace model

#endif
