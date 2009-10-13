

#ifndef _parser_IntConst_h_
#define _parser_IntConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

SPUG_RCPTR(IntConst);

// There's only one kind of integer constant - it can be specialized based on 
// context - e.g. 32 bit, 64 bit, signed versus unsigned...
class IntConst : public Expr {
    public:
        long val;

        IntConst(const TypeDefPtr &type, long val);
        
        virtual void emit(Context &context);
};

} // namespace parser

#endif
