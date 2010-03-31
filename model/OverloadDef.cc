// Copyright 2009 Google Inc.

#include "OverloadDef.h"

#include "Context.h"
#include "Expr.h"
#include "TypeDef.h"
#include "VarDefImpl.h"

using namespace std;
using namespace model;

OverloadDef *OverloadDef::Parent::getOverload(const OverloadDef *owner) const {
    
    // if we don't currently have an overload, try to get one from the parent 
    // context.
    if (!overload) {
        VarDefPtr varDef = context->lookUp(owner->name);
        if (varDef)
            overload = OverloadDefPtr::rcast(varDef);
    }
    return overload.get();
}

void OverloadDef::setImpl(FuncDef *func) {
    type = func->type;
    impl = func->impl;
}

void OverloadDef::flatten(OverloadDef::FuncList &flatFuncs) const {
    
    // first do all of the local functions
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        bool gotMatch = false;
        for (FuncList::const_iterator inner = flatFuncs.begin();
             inner != flatFuncs.end();
             ++inner
             )
            if ( (*inner)->matches((*iter)->args) ) {
                gotMatch = true;
                break;
            }
 
        // if the signature is not already in flatFuncs, add it.       
        if (!gotMatch)
            flatFuncs.push_back(iter->get());
    }
    
    // now flatten all of the parents
    for (ParentVec::const_iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        OverloadDef *parentOverload = parent->getOverload(this);
        if (parentOverload)
            parentOverload->flatten(flatFuncs);
    }
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
    
    for (ParentVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        OverloadDef *parentOverload = parent->getOverload(this);
        FuncDef *result = 
            parentOverload ? parentOverload->getMatch(context, args, convert) :
                             0;
        if (result)
            return result;
    }
    
    return 0;
}

FuncDef *OverloadDef::getSigMatch(const FuncDef::ArgVec &args) {
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if ((*iter)->matches(args))
            return iter->get();

    for (ParentVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        OverloadDef *parentOverload = parent->getOverload(this);
        FuncDef *result = 
            parentOverload ? parentOverload->getSigMatch(args) : 0;
        if (result)
            return result;
    }
    
    return 0;
}

void OverloadDef::addFunc(FuncDef *func) {
    if (funcs.empty()) setImpl(func);
    funcs.push_back(func);
}

void OverloadDef::addParent(Context *context) {
    parents.push_back(context);
}

bool OverloadDef::hasInstSlot() {
    return false;
}

bool OverloadDef::isSingleFunction() const {
    FuncList flatFuncs;
    flatten(flatFuncs);
    
    return flatFuncs.size() == 1;
}

void OverloadDef::createImpl() {
    if (!impl) {
        
        // get the impl from the first parent with one.
        for (ParentVec::iterator parent = parents.begin();
             parent != parents.end();
             ++parent
             ) {
            OverloadDef *parentOverload = parent->getOverload(this);
            if (parentOverload) {
                parentOverload->createImpl();
                if (parentOverload->impl) {
                    impl = parentOverload->impl;
                    break;
                }
            }
        }
    
        assert(impl);
    }
}

void OverloadDef::dump(ostream &out, const string &prefix) const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        (*iter)->dump(out, prefix);

    for (ParentVec::const_iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        OverloadDef *parentOverload = parent->getOverload(this);
        if (parentOverload)
            parentOverload->dump(out, prefix);
    }
}
