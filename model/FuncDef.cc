
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

bool FuncDef::matches(const vector<ExprPtr> &vals) {
    ArgVec::iterator arg;
    vector<ExprPtr>::const_iterator val;
    for (arg = args.begin(), val = vals.begin();
         arg != args.end() && val != vals.end();
         ++arg, ++val
         ) {
        if (!(*arg)->type->matches(*(*val)->type))
            return false;
    }

    // make sure that we checked everything in both lists   
    if (arg != args.end() || val != vals.end())
        return false;
    
    return true;
}

bool FuncDef::hasInstSlot() {
    return false;
}
