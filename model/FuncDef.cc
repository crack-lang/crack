
#include "FuncDef.h"

#include "Context.h"
#include "ArgDef.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace model;
using namespace std;

FuncDef::FuncDef(Flags flags, const std::string &name, size_t argCount) :
    // XXX need function types, but they'll probably be assigned after 
    // the fact.
    VarDef(0, name),
    flags(flags),
    args(argCount) {
}

bool FuncDef::matches(Context &context, const vector<ExprPtr> &vals, 
                      vector<ExprPtr> &newVals
                      ) {
    ArgVec::iterator arg;
    vector<ExprPtr>::const_iterator val;
    int i;
    for (arg = args.begin(), val = vals.begin(), i = 0;
         arg != args.end() && val != vals.end();
         ++arg, ++val, ++i
         ) {
        newVals[i] = (*val)->convert(context, (*arg)->type.get());
        if (!newVals[i])
            return false;
    }

    // make sure that we checked everything in both lists   
    if (arg != args.end() || val != vals.end())
        return false;
    
    return true;
}

bool FuncDef::matches(const ArgVec &other_args) {
    ArgVec::iterator arg;
    ArgVec::const_iterator other_arg;
    for (arg = args.begin(), other_arg = other_args.begin();
         arg != args.end() && other_arg != other_args.end();
         ++arg, ++other_arg
         ) {
        if (!(*arg)->type->matches(*(*other_arg)->type))
            return false;
    }

    // make sure that we checked everything in both lists   
    if (arg != args.end() || other_arg != other_args.end())
        return false;
    
    return true;
}

bool FuncDef::hasInstSlot() {
    return false;
}
