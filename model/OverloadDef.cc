
#include "OverloadDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace std;
using namespace model;

FuncDefPtr OverloadDef::getMatch(Context &context, vector<ExprPtr> &args) {
    vector<ExprPtr> newArgs(args.size());
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(context, args, newArgs)) {
            args = newArgs;
            return *iter;
        }
    
    return 0;
}

bool OverloadDef::matches(const FuncDef::ArgVec &args) {
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(args))
            return true;
    
    return false;
}

bool OverloadDef::hasInstSlot() {
    return false;
}
