
#ifndef _parser_StrConst_h_
#define _parser_StrConst_h_

#include "Context.h"
#include "Expr.h"

namespace model {

class StrConst : public Expr {
    public:
        std::string val;
        
        StrConst(const TypeDefPtr &type, const std::string &val);
        
        virtual void emit(Context &context);
};

} // namespace parser

#endif
