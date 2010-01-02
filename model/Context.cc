
#include "Context.h"

#include "builder/Builder.h"
#include "BuilderContextData.h"
#include "CleanupFrame.h"
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
    toplevel(false),
    emittingCleanups(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    globalData(parentContext ? parentContext->globalData : new GlobalData()),
    cleanupFrame(builder.createCleanupFrame(*this)) {
    
    if (parentContext)
        parents.push_back(parentContext);
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context::GlobalData *globalData
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    toplevel(false),
    emittingCleanups(false),
    returnType(TypeDefPtr(0)),
    globalData(globalData),
    cleanupFrame(builder.createCleanupFrame(*this)) {
}

Context::~Context() {}

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

OverloadDefPtr Context::aggregateOverloads(const std::string &varName,
                                           VarDef *resolvedVar
                                           ) {
    // do a symbol lookup if the caller hasn't done one for us
    VarDefPtr var = resolvedVar;
    if (!resolvedVar)
        var = lookUp(varName);

    // see if we've got a function definition
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
                           vector<ExprPtr> &args
                           ) {
    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName);
    if (!var)
        return 0;

    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef)
        return typeDef->context->lookUp("oper new", args);

    // otherwise, do the overload aggregation
    OverloadDefPtr overload = aggregateOverloads(varName, var.get());
    if (!overload)
        return 0;
    return overload->getMatch(*this, args);
}

void Context::addDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");
    
    // if there is an existing definition in this context, and the existing 
    // defintion and the new definition are both functions, create an overload 
    // aggregate.
    VarDefMap::iterator iter = defs.find(def->name);
    FuncDef *funcDef;
    if (iter != defs.end() && (funcDef = FuncDefPtr::cast(def)) && 
        (FuncDefPtr::rcast(iter->second) || 
         OverloadDefPtr::rcast(iter->second)
         )
        ) {
        OverloadDef *overloads;
        defs[def->name] = overloads = aggregateOverloads(def->name).get();
        overloads->funcs.push_front(funcDef);
    } else {
        defs[def->name] = def;
    }
    def->context = this;
}

void Context::replaceDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");
    def->context = this;
    defs[def->name] = def;
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

CleanupFramePtr Context::createCleanupFrame() {
    CleanupFramePtr frame = builder.createCleanupFrame(*this);
    frame->parent = cleanupFrame;
    cleanupFrame = frame;
    return frame;
}

void Context::closeCleanupFrame() {
    CleanupFramePtr frame = cleanupFrame;
    cleanupFrame = frame->parent;
    frame->close();
}
