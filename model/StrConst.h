
#ifndef _model_StrConst_h_
#define _model_StrConst_h_
    
#include "Context.h"
#include "Expr.h"

namespace model {

class StrConst : public Expr {
    public:
        std::string val;
        
        StrConst(TypeDef *type, const std::string &val);
        
        virtual ResultExprPtr emit(Context &context);
        virtual void writeTo(std::ostream &out) const;
};

} // namespace parser

#endif
