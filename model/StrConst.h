
#ifndef _model_StrConst_h_
#define _model_StrConst_h_
    
#include "Context.h"
#include "Expr.h"

namespace model {

class StrConst : public Expr {
    public:
        std::string val;
        
        StrConst(TypeDef *type, const std::string &val);
        
        virtual void emit(Context &context);
};

} // namespace parser

#endif
