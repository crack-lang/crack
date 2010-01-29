
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
        enum Flags {
            noFlags =0,  // so we can specify this
            method = 1,  // function is a method (has a receiver)
            virtualized = 2, // function is virtual
        } flags;
        
        typedef std::vector<ArgDefPtr> ArgVec;
        ArgVec args;

        FuncDef(Flags flags, const std::string &name, size_t argCount);
        
        /**
         * Returns true if 'args' matches the types of the functions 
         * arguments.
         */
        bool matches(Context &context, const std::vector<ExprPtr> &vals,
                     std::vector<ExprPtr> &newVals
                     );
        
        /**
         * Returns true if the arg list matches the functions args.
         */
        bool matches(const ArgVec &args);
        
        /**
         * Returns true if the function can be overriden.
         */
        bool isOverridable() const;
        
        virtual bool hasInstSlot();
        
        virtual void dump(std::ostream &out);
};

inline FuncDef::Flags operator |(FuncDef::Flags a, FuncDef::Flags b) {
    return static_cast<FuncDef::Flags>(static_cast<int>(a) | 
                                       static_cast<int>(b));
}
        

} // namespace model

#endif
