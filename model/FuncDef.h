
#ifndef _model_FuncDef_h_
#define _model_FuncDef_h_

#include <vector>
#include "VarDef.h"

namespace model {

SPUG_RCPTR(ArgDef);
SPUG_RCPTR(Expr);

SPUG_RCPTR(FuncDef);

class FuncDef : public VarDef {
    public:
        typedef std::vector<ArgDefPtr> ArgVec;
        ArgVec args;

        FuncDef(const std::string &name, size_t argCount);
        
        /**
         * Returns true if 'args' matches the types of the functions 
         * arguments.
         */
        bool matches(const std::vector<ExprPtr> &vals);
};

} // namespace model

#endif
