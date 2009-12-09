
#include "Context.h"

#include "builder/Builder.h"
#include "BuilderContextData.h"
#include "OverloadDef.h"
#include "StrConst.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    globalData(parentContext ? parentContext->globalData : new GlobalData()) {
    
    if (parentContext)
        parents.push_back(parentContext);
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context::GlobalData *globalData
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    returnType(TypeDefPtr(0)),
    globalData(globalData) {
}

ContextPtr Context::createSubContext(Scope newScope) {
    return new Context(builder, newScope, this);
}

ContextPtr Context::getClassContext() {
    if (scope == instance)
        return this;

    ContextPtr result;
    for (ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (result = (*iter)->getClassContext()) break;
    
    return result;
}

ContextPtr Context::getDefContext() {
    if (scope != composite)
        return this;
    
    ContextPtr result;
    for (ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (result = (*iter)->getDefContext()) break;
    
    return result;
}

void Context::createModule(const char *name) {
    builder.createModule(name);
}

namespace {
    inline void allocOverload(OverloadDefPtr &overload, const string &name) {
        if (!overload)
            overload = new OverloadDef(name);
    }
    
    inline void mergeOverloads(OverloadDef::FuncList &aggregator,
                               const OverloadDef::FuncList &newSubset
                               ) {
        for (OverloadDef::FuncList::const_iterator iter = newSubset.begin();
             iter != newSubset.end();
             ++iter
             ) 
            aggregator.push_front(*iter);
    }
}

OverloadDefPtr Context::aggregateOverloads(const std::string &varName) {
    // see if this is defined locally
    VarDefPtr var = lookUp(varName);
    FuncDef *func = 0;
    if (var) {
        // if it's already an overload, we're done
        OverloadDef *overload = OverloadDefPtr::rcast(var);
        if (overload)
            return overload;
        
        // make sure it's at least a function
        func = FuncDefPtr::rcast(var);
        if (!func)
            return 0;
    }

    OverloadDefPtr overloads;
    if (parents.size())
        for (ContextVec::iterator iter = parents.begin();
             iter != parents.end();
             ++iter
             ) {
            // XXX somebody somewhere needs to check for collisions
            
            // collect or merge the overloads of the new parent.
            OverloadDefPtr parentOverloads =
                (*iter)->aggregateOverloads(varName);
            if (parentOverloads)
                if (overloads)
                    mergeOverloads(overloads->funcs, parentOverloads->funcs);
                else
                    overloads = parentOverloads;
        }

    // if we got a local override, add it to the overloads
    if (func) {
        // make sure we've got one
        allocOverload(overloads, varName);
        
        // add it to the front of the list so that it will resolve before any 
        // parents with the same signature
        overloads->funcs.push_front(func);
    }
    
    // this could be overloads or null
    return overloads;
}
        

VarDefPtr Context::lookUp(const std::string &varName, bool recurse) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end())
        return iter->second;
    else if (recurse && parents.size())
        for (ContextVec::iterator parent_iter = parents.begin();
             parent_iter != parents.end();
             ++parent_iter
             ) {
            VarDefPtr def = (*parent_iter)->lookUp(varName);
            if (def)
                return def;
        }

    return 0;
}

FuncDefPtr Context::lookUp(const std::string &varName,
                           const vector<ExprPtr> &args
                           ) {
    OverloadDefPtr overload = aggregateOverloads(varName);
    if (!overload)
        return 0;
    return overload->getMatch(args);
}

void Context::addDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");
    defs[def->name] = def;
    def->context = this;
}

StrConstPtr Context::getStrConst(const std::string &value) {
    StrConstTable::iterator iter = globalData->strConstTable.find(value);
    if (iter != globalData->strConstTable.end()) {
        return iter->second;
    } else {
        // create a new one
        StrConstPtr strConst = builder.createStrConst(*this, value);
        globalData->strConstTable[value] = strConst;
        return strConst;
    }
}

void Context::emitCleanups(Context::Depth depth) {
    for (VarDefMap::iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         ) {
        VarDef *def = iter->second.get();
        VarDefPtr release;
        if (def->type && def->type->context && 
            (release = def->type->context->lookUp("release"))
            ) {
            // Create a release expression for the variable and emit it.
            FuncCallPtr releaseExpr = new FuncCall(FuncDefPtr::rcast(release));
            releaseExpr->receiver = new VarRef(def);
            releaseExpr->emit(*this);
        }
    }
    
    // do cleanups on the parent contexts
    if (depth != block)
        for (ContextVec::iterator iter = parents.begin();
             iter != parents.end();
             ++iter
             )
            if ((*iter)->scope == local)
                (*iter)->emitCleanups(depth);
}
