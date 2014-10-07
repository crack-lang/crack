// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Expr_h_
#define _model_Expr_h_

#include <vector>

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(ResultExpr);

SPUG_RCPTR(Expr);

// Base class for everything representing an expression.
class Expr : public spug::RCBase {
    public:
        TypeDefPtr type;
        
        Expr(TypeDef *type);
        
        ~Expr();

        /**
         * Returns the display name of the type.  This has the advantage of 
         * working for OverloadDefs, whose type is null.
         */
        std::string getTypeDisplayName() const;

        /**
         * Returns the expression type in a way that allows us to override it 
         * for OverloadDefs. Produces an error if the expression type is 
         * undefined.
         */        
        virtual TypeDefPtr getType(Context &context) const {
            // We can just return 'type' in the base case, as this is 
            // generally guaranteed to be defined.
            return type;
        }

        /**
         * Emit the expression in the given context.
         * 
         * Returns a result expression to be used for object lifecycle
         * management.
         */
        virtual ResultExprPtr emit(Context &context) = 0;

        /**
         * Emit the expression for use in a conditional context.
         * This defaults to calling Builder::emitTest().  Builder-derived 
         * classes should override in cases where there is a more appropriate 
         * path.
         */
        virtual void emitCond(Context &context);
        
        /**
         * Returns an expression that converts the expression to the specified 
         * type, null if it cannot be converted.
         */
        virtual ExprPtr convert(Context &context, TypeDef *type);

        /** Returns true if the expression is productive (see /notes.txt). */
        virtual bool isProductive() const;
        
        /**
         * Returns true if the expression changes type to adapt to the 
         * requirements of the context (integer and float constants).
         */
        virtual bool isAdaptive() const;
        
        /**
         * Write a representation of the expression to the stream.
         */
        virtual void writeTo(std::ostream &out) const = 0;
        
        /**
         * Do const folding on the expression.  Returns either the original 
         * expression object or a folded constant.
         * The base class version of this always returns null.  Derived 
         * classes may implement it for operations that support const folding.
         */
        virtual ExprPtr foldConstants();

        /**
         * Returns a new expression representing a function call to this 
         * expression with the given arguments.  Raises an error if the object 
         * can not be called with these arguments.
         * @param args The arg list.  May be modified in the course of the 
         *             call.
         */
        virtual ExprPtr makeCall(Context &context, 
                                 std::vector<ExprPtr> &args
                                 ) const;
        
        void dump() const;
};

inline std::ostream &operator <<(std::ostream &out, const Expr &expr) {
    expr.writeTo(out);
    return out;
}

} // namespace model

#endif
