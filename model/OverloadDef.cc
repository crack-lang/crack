
#include "OverloadDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace std;
using namespace model;

FuncDef *OverloadDef::getMatch(Context &context, vector<ExprPtr> &args,
                               bool convert
                               ) {
    vector<ExprPtr> newArgs(args.size());
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(context, args, newArgs, convert)) {
            if (convert)
                args = newArgs;
            return iter->get();
        }
    
    return 0;
}

FuncDef *OverloadDef::getSigMatch(const FuncDef::ArgVec &args) {
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(args))
            return iter->get();
    
    return 0;
}

void OverloadDef::addFunc(FuncDef *func) {
    funcs.insert(startOfParents++, func);
}

void OverloadDef::merge(OverloadDef &parent) {
    for (FuncList::iterator iter = parent.funcs.begin();
         iter != parent.funcs.end();
         ++iter
         )
        funcs.push_back(*iter);
    
    // make sure we didn't dislodge the parent pointer
    if (startOfParents == funcs.end())
        startOfParents = funcs.begin();
}

bool OverloadDef::hasInstSlot() {
    return false;
}
