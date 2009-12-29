

#ifndef _model_IntConst_h_
#define _model_IntConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(IntConst);

// There's only one kind of integer constant - it can be specialized based on 
// context - e.g. 32 bit, 64 bit, signed versus unsigned...
class IntConst : public Expr {
    public:
        long val;

        IntConst(TypeDef *type, long val);
        
        virtual void emit(Context &context);
        virtual void writeTo(std::ostream &out) const;        
};

} // namespace parser

#endif
