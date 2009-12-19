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
    private:
        AssignExpr(Expr *aggregate, VarDef *var, Expr *value);

    public:
        ExprPtr aggregate, value;
        VarDefPtr var;

        /**
         * Create a new AssignExpr, check for errors.
         * @param varName the variable name token, used for error checking.
         * @param aggregate the aggregate that the variable is a member of
         * @param var the variable
         * @param value the value to be assigned to the variable.
         */
        static AssignExprPtr create(const parser::Token &varName,
                                    Expr *aggregate,
                                    VarDef *var, 
                                    Expr *value
                                    );

        /**
         * Create a new AssignExpr, check for errors.  Just like the other 
         * create() method only for non-instance variables (statics, globals 
         * and locals)
         * 
         * @param varName the variable name token, used for error checking.
         * @param var the variable
         * @param value the value to be assigned to the variable.
         */
        static AssignExprPtr create(const parser::Token &tok,
                                    VarDef *var, 
                                    Expr *value
                                    ) {
            return create(tok, 0, var, value);
        }

        /** Emit the expression in the given context. */
        virtual void emit(Context &context);
};

} // namespace model

#endif
