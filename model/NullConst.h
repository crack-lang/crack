
#ifndef _model_NullConst_h_
#define _model_NullConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(NullConst);

// A null constant.  Like IntConst, this will morph to adapt to the 
// corresponding type/width etc.
class NullConst : public Expr {
    public:
        NullConst(TypeDef *type) : Expr(type) {}
        
        virtual void emit(Context &context);
        
        virtual ExprPtr convert(Context &context, TypeDef *newType);
        virtual void writeTo(std::ostream &out) const;        
};

} // namespace parser

#endif
