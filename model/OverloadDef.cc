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
         )
        (*parent)->flatten(flatFuncs);
}

FuncDef *OverloadDef::getMatch(Context &context, vector<ExprPtr> &args,
                               FuncDef::Convert convertFlag,
                               bool allowOverrides
                               ) {
    vector<ExprPtr> newArgs(args.size());
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter) {
        
        // ignore the function if it is a virtual override and we're not 
        // looking for those
        if (!allowOverrides && (*iter)->isVirtualOverride())
            continue;
        
        // see if the function matches
        if ((*iter)->matches(context, args, newArgs, convertFlag)) {
            if (convertFlag != FuncDef::noConvert)
                args = newArgs;
            return iter->get();
        }
    }
    
    for (ParentVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        FuncDef *result = (*parent)->getMatch(context, args, convertFlag,
                                              allowOverrides
                                              );
        if (result)
            return result;
    }
    
    return 0;
}

FuncDef *OverloadDef::getMatch(Context &context, std::vector<ExprPtr> &args,
                               bool allowOverrides
                               ) {
    
    // see if we have any adaptive arguments and if all of them are adaptive.
    bool someAdaptive = false, allAdaptive = true;
    for (vector<ExprPtr>::iterator iter = args.begin();
         iter != args.end();
         ++iter
         )
        if ((*iter)->isAdaptive())
            someAdaptive = true;
        else
            allAdaptive = false;
    
    // if any of the arguments are adpative, convert the adaptive arguments.
    FuncDef::Convert convertFlag = FuncDef::noConvert;
    if (someAdaptive)
        convertFlag = FuncDef::adapt;

    // if _all_ of the arguments are adaptive, 
    if (allAdaptive)
        convertFlag = FuncDef::adaptSecondary;
    
    FuncDef *result = getMatch(context, args, convertFlag, allowOverrides);
    if (!result)
        result = getMatch(context, args, FuncDef::convert, allowOverrides);
    return result;
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
        FuncDef *result = (*parent)->getSigMatch(args);
        if (result)
            return result;
    }
    
    return 0;
}

FuncDef *OverloadDef::getNoArgMatch(bool acceptAlias) {
    
    // check the local functions.
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        if ((*iter)->args.empty() && 
            (acceptAlias || (*iter)->getOwner() == owner)
            )
            return iter->get();

    // check delegated functions.
    for (ParentVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        FuncDef *result = (*parent)->getNoArgMatch(acceptAlias);
        if (result)
            return result;
    }
    
    return 0;
}

OverloadDefPtr OverloadDef::createAlias() {
    OverloadDefPtr alias = new OverloadDef(name);
    flatten(alias->funcs);
    return alias;
}

void OverloadDef::addFunc(FuncDef *func) {
    if (funcs.empty()) setImpl(func);
    funcs.push_back(func);
}

void OverloadDef::addParent(OverloadDef *parent) {
    parents.push_back(parent);
}

bool OverloadDef::hasParent(OverloadDef *parent) {
    for (ParentVec::iterator iter = parents.begin(); iter != parents.end();
         ++iter
         )
        if (iter->get() == parent)
            return true;
    return false;
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
            (*parent)->createImpl();
            if ((*parent)->impl) {
                impl = (*parent)->impl;
                break;
            }
        }
    
        assert(impl);
    }
}

bool OverloadDef::isConstant() {
    return true;
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
         )
        (*parent)->dump(out, prefix);
}
