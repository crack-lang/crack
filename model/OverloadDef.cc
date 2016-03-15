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
#include "spug/check.h"

#include "Context.h"
#include "Deserializer.h"
#include "Expr.h"
#include "FuncCall.h"  // just so we can "out << args"
#include "OverloadAliasTreeNode.h"
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
    
    // first do all of the local functions.  It's ok to have multiple
    // functions with the same signature in this list, when we do lookups we
    // will traverse the list in the normal order and do the right thing with
    // respect to overrides.
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        flatFuncs.push_back(iter->get());
    
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
        SPUG_FOR(FuncList, iter, alias->funcs) {
            if (!(*iter)->isImportable(owner, (*iter)->name))
                (*iter)->exposed = true;
        }
    }
    return alias;
}

bool OverloadDef::getSecondOrderImports(OverloadDef::FuncList &results,
                                        ModuleDef *module
                                        ) const {
    bool gotMix = false;
    SPUG_FOR(FuncList, iter, funcs) {
        if ((*iter)->getOwner() != module)
            results.push_back(*iter);
        else
            gotMix = true;
    }

    SPUG_FOR(ParentVec, parent, parents)
        gotMix |= (*parent)->getSecondOrderImports(results, module);

    return gotMix;
}

bool OverloadDef::privateVisibleTo(Namespace *ns) const {
    SPUG_FOR(FuncList, iter, funcs) {
        // If the function is owned by a type but the namespace isn't scoped
        // to the type, we fail.
        Namespace *owner = (*iter)->getOwner();
        if (TypeDefPtr::cast(owner) && !ns->isScopedTo(owner))
            return false;
    }

    SPUG_FOR(ParentVec, parent, parents) {
        if (!(*parent)->privateVisibleTo(ns))
            return false;
    }

    return true;
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

void OverloadDef::addParent(OverloadDef *parent, bool before) {
    // When inserting before, we don't check for intermediates.
    if (before) {
        vector<OverloadDefPtr> newParents;
        newParents.push_back(parent);
        SPUG_FOR(ParentVec, iter, parents)
            newParents.push_back(*iter);
        parents = newParents;
        return;
    }

    // Replace any parents that the new parent derives from (to preserve the
    // correct lookup order, we should be discovering those parents through
    // the overload)
    // Note: I'm not convinced that this code is correct.  The relationship
    // between overloads and their namespaces is complicated, and the
    // approaches that seemed correct while fixing the most recent problem
    // (see 235_method_resolution_order) didn't work.  This code, however,
    // does.
    for(ParentVec::iterator iter = parents.begin(); iter != parents.end(); ) {
        if (parent->hasAncestor(iter->get())) {
            iter = parents.erase(iter);
            parents.insert(iter, parent);
            return;
        } else {
            ++iter;
        }
    }

    parents.push_back(parent);
}

void OverloadDef::collectAncestors(Namespace *ns) {
    bool classPrivate = name.substr(0, 2) == "__" && TypeDefPtr::cast(ns);
    NamespacePtr parent;
    for (unsigned i = 0; parent = ns->getParent(i++);) {

        // If the overload is private and the parent is a type, ignore the
        // parent (we don't want to absorb his overloads).
        if (classPrivate && TypeDefPtr::rcast(parent))
            continue;

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

bool OverloadDef::hasAncestor(OverloadDef *parent) {
    if (this == parent)
        return true;

    for (ParentVec::iterator iter = parents.begin(); iter != parents.end();
         ++iter
         ) {
        if ((*iter)->hasAncestor(parent))
            return true;
    }

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
    
    // the overload is importable if all of its functions are importable.
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        if ((*iter)->getOwner() != module)
            return false;

    return true;
}

bool OverloadDef::isImportable(const Namespace *ns, 
                               const std::string &name
                               ) const {
    // the overload is importable if any of its functions are importable.
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        if ((*iter)->isImportable(ns, name))
            return true;
    }

    SPUG_FOR(ParentVec, iter, parents) {
        if ((*iter)->isImportable(ns, name))
            return true;
    }

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

bool OverloadDef::isImportedIn(const Context &context) const {
    // Functions in an overload should either be all imported or all local, so
    // as soon as we find a function we can make this determination.
    SPUG_FOR(FuncList, iter, funcs) {
        ModuleDefPtr realModule = context.ns->getRealModule();
        if ((*iter)->getOwner()->getRealModule() == realModule)
            return false;
        else
            return true;
    }

    SPUG_FOR(ParentVec, iter, parents) {
        if ((*iter)->isImportedIn(context))
            return true;
        else
            return false;
    }

    // No parents and no functions - we should never get here.
    SPUG_CHECK(false,
               "isImportedIn() called on overload " << name <<
                " which has no parents and no functions."
               );
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
    Namespace *owner = getOwner();
    cerr << prefix << "overload " << name << " in " <<
        (owner ? owner->getNamespaceName() : "null") <<
        " {" << endl;
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         )
        (*iter)->dump(out, prefix + "  ");

    for (ParentVec::const_iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         )
        (*parent)->dump(out, prefix + "  ");
    cerr << prefix << "}" << endl;
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

void OverloadDef::serialize(Serializer &serializer, bool writeKind,
                            const Namespace *ns
                            ) const {

    // Build a list of the overloads up front.
    vector<FuncDef *> overloads;
    for (FuncList::const_iterator iter = funcs.begin();
         iter != funcs.end();
         ++iter
         ) {
        // Aliases get serialized after everything else (see AliasTreeNode).
        if ((*iter)->isSerializable() && !(*iter)->isAliasIn(*this))
            overloads.push_back(iter->get());
    }

    if (writeKind)
        serializer.write(Serializer::overloadId, "kind");
    serializer.write(name, "name");
    serializer.write(overloads.size(), "#overloads");

    SPUG_FOR(vector<FuncDef *>, iter, overloads)
        (*iter)->serialize(serializer);
}

OverloadDefPtr OverloadDef::deserialize(Deserializer &deser,
                                        Namespace *owner,
                                        bool aliases
                                        ) {
    string name = deser.readString(Serializer::modNameSize, "name");
    OverloadDefPtr ovld;
    
    if (Serializer::trace)
        cerr << "# begin overload " << name << endl;

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
        FuncDefPtr func;
        if (aliases)
            func = FuncDef::deserializeAlias(deser);
        else
            func = FuncDef::deserialize(deser, name);
        if (!func->getOwner())
            func->setOwner(owner);
        ovld->addFunc(func.get());
    }

    if (Serializer::trace)
        cerr << "# end overload " << name << endl;

    return ovld;
}

namespace {
    // Returns true if the overload def should include the function in its 
    // aliase tree node.
    bool shouldInclude(bool privateAliases, const OverloadDef *ovld,
                       OverloadDef::FuncList::const_iterator entry
                       ) {

        // No non-aliases or builtins ever.
        if (!(*entry)->isAliasIn(*ovld) || (*entry)->flags & FuncDef::builtin)
            return false;

        bool importable = (*entry)->isImportable(ovld->getOwner(),
                                                 ovld->name
                                                 );
        if (privateAliases)
            return !importable;
        else
            return importable;
    }
}

OverloadAliasTreeNodePtr OverloadDef::getAliasTree(bool privateAliases) {
    OverloadAliasTreeNodePtr result;
    SPUG_FOR(FuncList, iter, funcs) {
        if (shouldInclude(privateAliases, this, iter)) {
            if (!result)
                result = new OverloadAliasTreeNode(this);
            result->addAlias(iter->get());
        }
    }

    // We don't need to collect aliases from parent contexts: those should get
    // serialized in their own namespaces.
    return result;
}

bool OverloadDef::containsAliases(bool privateAliases) const {
    SPUG_FOR(FuncList, iter, funcs) {
        if (shouldInclude(privateAliases, this, iter))
            return true;
    }

    return false;
}
