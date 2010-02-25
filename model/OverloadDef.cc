// Copyright 2009 Google Inc.

#include "OverloadDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace std;
using namespace model;

void OverloadDef::setImpl(FuncDef *func) {
    type = func->type;
    impl = func->impl;
}

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
    if (funcs.empty()) setImpl(func);
    startOfParents = funcs.insert(startOfParents, func);
    startOfParents++;
}

void OverloadDef::merge(OverloadDef &parent) {
    for (FuncList::iterator iter = parent.funcs.begin();
         iter != parent.funcs.end();
         ++iter
         ) {
        if (funcs.empty()) setImpl(iter->get());
        funcs.push_back(*iter);
    }
    
    // make sure we didn't dislodge the parent pointer
    if (startOfParents == funcs.end())
        startOfParents = funcs.begin();
}

bool OverloadDef::hasInstSlot() {
    return false;
}

void OverloadDef::dump(ostream &out, const string &prefix) const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        (*iter)->dump(out, prefix);
}
