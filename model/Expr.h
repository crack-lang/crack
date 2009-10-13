
#ifndef _model_Expr_h_
#define _model_Expr_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(Expr);

// Base class for everything representing an expression.
class Expr : public spug::RCBase {
    public:
        TypeDefPtr type;
        
        Expr(const TypeDefPtr &type) : type(type) {}

        /** Emit the expression in the given context. */
        virtual void emit(Context &context) = 0;
};

} // namespace model

#endif
