// Copyright 2009-2012 Google Inc.
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "OverloadDef.h"

#include "spug/stlutil.h"
#include "spug/StringFmt.h"

#include "Context.h"
#include "Deserializer.h"
#include "Expr.h"
#include "FuncCall.h"  // just so we can "out << args"
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
                               ) const {
    vector<ExprPtr> newArgs(args.size());
    for (FuncList::const_iterator iter = funcs.begin();
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
    
    for (ParentVec::const_iterator parent = parents.begin();
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
                               ) const {
    
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

FuncDef *OverloadDef::getMatch(TypeDef *funcType) const {
    SPUG_FOR(FuncList, iter, funcs) {
        if ((*iter)->type->isDerivedFrom(funcType))
            return iter->get();
    }
    
    SPUG_FOR(ParentVec, iter, parents) {
        FuncDef *result = (*iter)->getMatch(funcType);
        if (result)
            return result;
    }
    
    return 0;
}

FuncDef *OverloadDef::getSigMatch(const FuncDef::ArgVec &args,
                                  bool matchNames
                                  ) const {
    SPUG_FOR(FuncList, iter, funcs) {
        if (!matchNames && (*iter)->matches(args) ||
            matchNames && (*iter)->matchesWithNames(args)
            )
            return iter->get();
    }

    SPUG_FOR(ParentVec, parent, parents) {
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

OverloadDefPtr OverloadDef::createAlias(bool exposeAll) {
    OverloadDefPtr alias = new OverloadDef(name);
    alias->type = type;
    alias->impl = impl;
    flatten(alias->funcs);
    if (exposeAll) {
        SPUG_FOR(FuncList, iter, funcs) {
            if (!(*iter)->isImportable(owner, (*iter)->name))
                (*iter)->exposed = true;
        }
    }
    return alias;
}

pair<bool, bool> OverloadDef::hasAliasesAndNonAliases() const {
    bool gotAliases = false, gotNonAliases = false;
    SPUG_FOR(FuncList, iter, funcs) {
        if ((*iter)->isAliasIn(*this)) {
            gotAliases = true;
            if (gotNonAliases) break;
        } else {
            gotNonAliases = true;
            if (gotAliases) break;
        }
    }
    return std::make_pair(gotAliases, gotNonAliases);
}

bool OverloadDef::hasExposedFuncs() const {
    SPUG_FOR(FuncList, iter, funcs) {
        if ((*iter)->exposed)
            return true;
    }
    return false;
}

void OverloadDef::addFunc(FuncDef *func) {
    if (funcs.empty()) setImpl(func);
    funcs.push_back(func);
}

void OverloadDef::addParent(OverloadDef *parent) {
    parents.push_back(parent);
}

void OverloadDef::collectAncestors(Namespace *ns) {
    NamespacePtr parent;
    for (unsigned i = 0; parent = ns->getParent(i++);) {
        VarDefPtr var = parent->lookUp(name, false);
        OverloadDefPtr parentOvld;
        if (!var) {
            // the parent does not have this overload.  Check the next level.
            collectAncestors(parent.get());
        } else {
            parentOvld = OverloadDefPtr::rcast(var);
            // if there is a variable of this name but it is not an overload, 
            // we have a situation where there is a non-overload definition in 
            // an ancestor namespace that will block resolution of the 
            // overloads in all derived namespaces.  This is a bad thing, 
            // but not something we want to deal with here.

            if (parentOvld)
                addParent(parentOvld.get());
        }
    }
}

bool OverloadDef::hasParent(OverloadDef *parent) {
    for (ParentVec::iterator iter = parents.begin(); iter != parents.end();
         ++iter
         )
        if (iter->get() == parent)
            return true;
    return false;
}

FuncDefPtr OverloadDef::getFuncDef(Context &context, 
                                   std::vector<ExprPtr> &args,
                                   bool allowOverrides
                                   ) const {
    FuncDefPtr result = getMatch(context, args, allowOverrides);
    if (!result) {
        ostringstream msg;
        msg << "No method exists matching " << name <<  "(" << args << ")";
        context.maybeExplainOverload(msg, name, getOwner());
        context.error(msg.str());
    }
    
    // For functions initially defined as abstract, the normal method 
    // resolution (which we want) gives us the abstract function.  But we 
    // don't want that here because we need to give an error on explicitly 
    // scoped abstract functions.  So we have to go through and look the 
    // function up again now that we've nailed down the signature to get the 
    // implementation if there is one.
    if (result->flags & FuncDef::abstract)
        result = getSigMatch(result->args, /* matchNames */ true);
    
    return result;
}

bool OverloadDef::hasInstSlot() const {
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

bool OverloadDef::isImportableFrom(ModuleDef *module, 
                                   const string &impName
                                   ) const {
    if (module->exports.find(impName) != module->exports.end())
        return true;
    
    // the overload is importable if any of its functions are importable.
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        if ((*iter)->getOwner() == module)
            return true;

    return false;
}

bool OverloadDef::isImportable(const Namespace *ns, 
                               const std::string &name
                               ) const {
    // the overload is importable if any of its functions are importable.
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        if ((*iter)->isImportable(ns, name))
            return true;

    return false;
}

bool OverloadDef::isUsableFrom(const Context &context) const {
    // Overloads are always usable because they have an address (we can say 
    // x := A.f).  We decide at call time if they need (and have) a receiver.
    return true;
}

bool OverloadDef::needsReceiver() const {
    // The overload doesn't need a receiver unless all of the methods do.
    // (Not sure if this is actually useful).
    SPUG_FOR(FuncList, iter, funcs)
        if (!(*iter)->needsReceiver())
            return false;

    // Check the parents.    
    SPUG_FOR(ParentVec, iter, parents)
        if (!(*iter)->needsReceiver())
            return false;
    
    return true;
}

bool OverloadDef::isSerializable() const {
    if (!VarDef::isSerializable())
        return false;
    else {
        return hasSerializableFuncs();
    }
}

bool OverloadDef::isSingleFunction() const {
    FuncList flatFuncs;
    flatten(flatFuncs);
    
    return flatFuncs.size() == 1;
}

FuncDefPtr OverloadDef::getSingleFunction() const {
    FuncList flatFuncs;
    flatten(flatFuncs);

    if (flatFuncs.size() == 1)
        return flatFuncs.front();
    else
        return 0;
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

bool OverloadDef::hasSerializableFuncs() const {
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        if ((*iter)->isSerializable())
            return true;
    }
    return false;
}

namespace {
    template <void (*T)(Serializer &, const FuncDef &)>
    void serializeFuncVec(Serializer &serializer, const vector<FuncDef *> &funcs, 
                          bool writeKind,
                          const string &name
                          ) {
        if (writeKind)
            serializer.write(Serializer::overloadId, "kind");
        serializer.write(name, "name");
        serializer.write(funcs.size(), "#overloads");
        SPUG_FOR(vector<FuncDef *>, iter, funcs)
                T(serializer, **iter);
    }
    
    void serializeFunc(Serializer &serializer, const FuncDef &funcDef) {
        funcDef.serialize(serializer);
    }
    
    void serializeFuncAlias(Serializer &serializer, const FuncDef &funcDef) {
        funcDef.serializeAlias(serializer);
    }
}

void OverloadDef::serializeAlias(Serializer &serializer,
                                 const string &name
                                 ) const {
    // Build a list of the aliases up front
    vector<FuncDef *> aliases;
    SPUG_FOR(FuncList, iter, funcs) {
        // We don't check for serializable here, aliases are assumed to always 
        // be serializable.
        if ((*iter)->isAliasIn(*this))
            aliases.push_back(iter->get());
    }

    serializeFuncVec<serializeFuncAlias>(serializer, aliases, /* writeKind */ true, name);
}

void OverloadDef::serialize(Serializer &serializer, bool writeKind,
                            const Namespace *ns
                            ) const {

    // Build a list of the overloads up front.
    vector<FuncDef *> overloads;
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        if ((*iter)->isSerializable() && !(*iter)->isAliasIn(*this))
            overloads.push_back(iter->get());
    }

    serializeFuncVec<serializeFunc>(serializer, overloads, writeKind, name);
}

OverloadDefPtr OverloadDef::deserialize(Deserializer &deser,
                                        Namespace *owner
                                        ) {
    string name = deser.readString(Serializer::modNameSize, "name");
    OverloadDefPtr ovld;
    
    // In order to deal with local aliases (which need to be defined after the
    // functions they reference) overloads can be serialized multiple times, 
    // so if the overload is already defined in the namespace, just use the 
    // existing one.
    if (!(ovld = owner->lookUp(name, false))) {
        ovld = new OverloadDef(name);
        ovld->type = deser.context->construct->overloadType;
        ovld->collectAncestors(owner);
    }
    int size = deser.readUInt("#overloads");
    for (int i = 0; i < size; ++i) {
        FuncDefPtr func = FuncDef::deserialize(deser, name);
        if (!func->getOwner())
            func->setOwner(owner);
        ovld->addFunc(func.get());
    }
    return ovld;
}
