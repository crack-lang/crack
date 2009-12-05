
#include "OverloadDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace std;
using namespace model;

FuncDefPtr OverloadDef::getMatch(const vector<ExprPtr> &args) {
    for (FuncVec::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(args))
            return *iter;
    
    return 0;
}

bool OverloadDef::hasInstSlot() {
    return false;
}
