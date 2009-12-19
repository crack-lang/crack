
#ifndef _model_OverloadDef_h_
#define _model_OverloadDef_h_

#include <list>

#include "FuncDef.h"

namespace model {

SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);

SPUG_RCPTR(OverloadDef);

/** An overloaded function. */
class OverloadDef : public VarDef {
    public:
        typedef std::list<FuncDefPtr> FuncList;
        FuncList funcs;

        OverloadDef(const std::string &name) :
            // XXX need function types, but they'll probably be assigned after 
            // the fact.
            VarDef(0, name) {
        }
        
        /**
         * Returns the overload matching the given args. This will modify 
         * "args" if there are conversions to be applied.
         */
        FuncDefPtr getMatch(Context &context, std::vector<ExprPtr> &args);
        
        /** 
         * Returns true if the overload includeds a signature for the 
         * specified argument list.
         */
        bool matches(const FuncDef::ArgVec &args);
        
        bool hasInstSlot();
};

} // namespace model

#endif
