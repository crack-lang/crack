// Copyright 2009-2012 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "OverloadDef.h"

#include "Context.h"
#include "Deserializer.h"
#include "Expr.h"
#include "Serializer.h"
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

FuncDef *OverloadDef::getSigMatch(const FuncDef::ArgVec &args,
                                  bool matchNames
                                  ) {
    for (FuncList::iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter)
        if (!matchNames && (*iter)->matches(args) ||
            matchNames && (*iter)->matchesWithNames(args)
            )
            return iter->get();

    for (ParentVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        FuncDef *result = (*parent)->getSigMatch(args, matchNames);
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
    alias->type = type;
    alias->impl = impl;
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

bool OverloadDef::isStatic() const {
    FuncList flatFuncs;
    flatten(flatFuncs);
    assert((flatFuncs.size() == 1) && 
           "isStatic() check applied to a multi-function overload"
           );
    return flatFuncs.front()->isStatic();
}

bool OverloadDef::isSerializable(const Namespace *ns) const {
    if (!VarDef::isSerializable(ns))
        return false;
    else
        return hasSerializableFuncs(ns);
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

void OverloadDef::display(ostream &out, const string &prefix) const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        (*iter)->display(out, prefix + "  ");
        out << '\n';
    }

    for (ParentVec::const_iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        (*parent)->display(out, prefix);
    }
}

void OverloadDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        (*iter)->addDependenciesTo(mod, added);
    }
}

bool OverloadDef::hasSerializableFuncs(const Namespace *ns) const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        if ((*iter)->isSerializable(ns))
            return true;
    }
}

void OverloadDef::serialize(Serializer &serializer, bool writeKind,
                            const Namespace *ns
                            ) const {

    // calculate the number of functions to serialize (we don't serialize 
    // builtins)
    int size = 0;
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        if ((*iter)->isSerializable(ns))
            ++size;

    if (writeKind)
        serializer.write(Serializer::overloadId, "kind");
    serializer.write(name, "name");
    
    serializer.write(funcs.size(), "#overloads");
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        if ((*iter)->isSerializable(ns))
            (*iter)->serialize(serializer, false, ns);
    }
}

OverloadDefPtr OverloadDef::deserialize(Deserializer &deser,
                                        Namespace *owner
                                        ) {
    string name = deser.readString(Serializer::modNameSize, "name");
    OverloadDefPtr ovld = new OverloadDef(name);
    int size = deser.readUInt("#overloads");
    for (int i = 0; i < size; ++i) {
        FuncDefPtr func = FuncDef::deserialize(deser, name);
        func->setOwner(owner);
        ovld->addFunc(func.get());
    }
    return ovld;
}