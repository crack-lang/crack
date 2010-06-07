// Copyright 2009 Google Inc.

#include "Context.h"

#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "parser/Token.h"
#include "parser/Location.h"
#include "parser/ParseError.h"
#include "BuilderContextData.h"
#include "CleanupFrame.h"
#include "ArgDef.h"
#include "Branchpoint.h"
#include "IntConst.h"
#include "ModuleDef.h"
#include "OverloadDef.h"
#include "StrConst.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;

void Context::storeDef(VarDef *def) {
    FuncDef *funcDef;
    if (funcDef = FuncDefPtr::cast(def)) {
        OverloadDefPtr overloads = getOverload(def->name);
        overloads->addFunc(funcDef);
    } else {        
        defs[def->name] = def;
        if (scope == instance && def->hasInstSlot())
            ordered.push_back(def);
    }
}

Context::GlobalData::GlobalData() : 
    objectType(0), stringType(0), staticStringType(0) {
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext
                 ) :
    builder(builder),
    scope(scope),
    complete(false),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
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
    terminal(false),
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

ContextPtr Context::getToplevel() {
    if (toplevel)
        return this;
    
    ContextPtr result;
    for (ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (result = (*iter)->getToplevel()) break;
    
    return result;
}

ContextPtr Context::getParent() {
    assert(parents.size() == 1);
    return parents[0];
}

bool Context::encloses(const Context &other) const {
    if (this == &other)
        return true;
    
    for (ContextVec::const_iterator iter = other.parents.begin();
         iter != other.parents.end();
         ++iter
         )
        if (encloses(**iter))
            return true;
    return false;
}

ModuleDefPtr Context::createModule(const string &name) {
    return builder.createModule(*this, name);
}

OverloadDefPtr Context::getOverload(const std::string &varName) {
    // see if the name exists in the current context
    OverloadDefPtr overloads = lookUp(varName, false);
    if (overloads)
        return overloads;

    overloads = new OverloadDef(varName);
    overloads->type = globalData->overloadType;
    
    // merge in the overloads from the parents
    if (parents.size())
        for (ContextVec::iterator iter = parents.begin();
             iter != parents.end();
             ++iter
             ) {
            overloads->addParent(iter->get());
        }

    if (overloads) {
        defs[varName] = overloads;
        overloads->context = this;
    }

    return overloads;
}

VarDefPtr Context::lookUp(const std::string &varName, bool recurse) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end()) {
        return iter->second;
    } else if (recurse && parents.size()) {
        
        // try to find the definition in the parents
        VarDefPtr def;
        for (ContextVec::iterator parent_iter = parents.begin();
             parent_iter != parents.end();
             ++parent_iter
             )
            if (def = (*parent_iter)->lookUp(varName))
                break;
        
        // if we got an overload, we need to create an overload in this 
        // context.
        OverloadDef *overload = OverloadDefPtr::rcast(def);
        if (overload)
            return getOverload(varName);
        else
            return def;
    }

    return 0;
}

FuncDefPtr Context::lookUp(Context &context,
                           const std::string &varName,
                           vector<ExprPtr> &args
                           ) {
    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName);
    if (!var)
        return 0;
    
    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef) {
        FuncDefPtr operNew =
            typeDef->context->lookUp(context, "oper new", args);

        // make sure we got it, and we didn't inherit it
        if (!operNew || operNew->context != typeDef->context.get())
            return 0;
        
        return operNew;
    }
    
    // if this is an overload, get the function from it.
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return 0;
    return overload->getMatch(context, args);
}

FuncDefPtr Context::lookUpNoArgs(const std::string &name, bool acceptAlias) {
    OverloadDefPtr overload = getOverload(name);
    if (!overload)
        return 0;

    // we can just check for a signature match here - cheaper and easier.
    FuncDef::ArgVec args;
    FuncDefPtr result = overload->getNoArgMatch(acceptAlias);
    return result;
}

void Context::addDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");

    storeDef(def);
    def->context = this;
}

void Context::removeDef(VarDef *def) {
    assert(!OverloadDefPtr::cast(def));
    VarDefMap::iterator iter = defs.find(def->name);
    assert(iter != defs.end());
    defs.erase(iter);
}

void Context::addAlias(VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->context);

    // overloads should never be aliased - otherwise the new context could 
    // extend them.
    OverloadDef *overload = OverloadDefPtr::cast(def);
    if (overload)
        storeDef(overload->createChild().get());
    else
        storeDef(def);
}

void Context::addAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->context);
    defs[name] = def;
}

void Context::replaceDef(VarDef *def) {
    assert(!def->context);
    assert(scope != composite && "defining a variable in a composite scope.");
    assert(!def->hasInstSlot() && 
           "Attempted to replace an instance variable, this doesn't work "
           "because it won't change the 'ordered' vector."
           );
    def->context = this;
    defs[def->name] = def;
}

ExprPtr Context::getStrConst(const std::string &value, bool raw) {
    
    // look up the raw string constant
    StrConstPtr strConst;
    StrConstTable::iterator iter = globalData->strConstTable.find(value);
    if (iter != globalData->strConstTable.end()) {
        strConst = iter->second;
    } else {
        // create a new one
        strConst = builder.createStrConst(*this, value);
        globalData->strConstTable[value] = strConst;
    }
    
    // if we don't have a StaticString type yet (or the caller wants a raw
    // bytestr), we're done.
    if (raw || !globalData->staticStringType)
        return strConst;
    
    // create the "new" expression for the string.
    vector<ExprPtr> args;
    args.push_back(strConst);
    args.push_back(builder.createIntConst(*this, value.size(),
                                          globalData->uintType.get()
                                          )
                   );
    FuncDefPtr newFunc =
        globalData->staticStringType->context->lookUp(*this, "oper new", args);
    FuncCallPtr funcCall = builder.createFuncCall(newFunc.get());
    funcCall->args = args;
    return funcCall;    
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

void Context::emitVarDef(TypeDef *type, const parser::Token &tok, 
                         Expr *initializer
                         ) {
    
    // if the definition context is an instance context, make sure that we 
    // haven't generated any constructors.
    ContextPtr defCtx = getDefContext();
    if (defCtx->scope == Context::instance && 
         defCtx->returnType->initializersEmitted
        ) {
        parser::Location loc = tok.getLocation();
        throw parser::ParseError(SPUG_FSTR(loc.getName() << ':' << 
                                            loc.getLineNumber() << 
                                            ": Adding an instance variable "
                                            "after 'oper init' has been "
                                            "defined."
                                           )
                         );
    }

    createCleanupFrame();
    VarDefPtr varDef = type->emitVarDef(*this, tok.getData(), initializer);
    closeCleanupFrame();
    defCtx->addDef(varDef.get());
    cleanupFrame->addCleanup(varDef.get());
}

void Context::setBreak(Branchpoint *branch) {
    breakBranch = branch;
}

void Context::setContinue(Branchpoint *branch) {
    continueBranch = branch;
}

Branchpoint *Context::getBreak() {
    if (breakBranch)
        return breakBranch.get();
    
    // don't attempt to propagate out of an execution scope
    if (!toplevel && !parents.empty()) {
        assert(parents.size() == 1 && "local scope has more than one parent");
        return parents[0]->getBreak();
    } else {
        return 0;
    }
}

Branchpoint *Context::getContinue() {
    if (continueBranch)
        return continueBranch.get();

    // don't attempt to propagate out of an execution scope
    if (!toplevel && !parents.empty()) {
        assert(parents.size() == 1 && "local scope has more than one parent");
        return parents[0]->getContinue();
    } else {
        return 0;
    }
}

void Context::dump(ostream &out, const std::string &prefix) const {
    switch (scope) {
        case module: out << "module "; break;
        case instance: out << "instance "; break;
        case local: out << "local "; break;
        case composite: out << "composite "; break;
        default: out << "UNKNOWN ";
    }
    out << "{\n";
    string childPfx = prefix + "  ";
    for (ContextVec::const_iterator parentIter = parents.begin();
         parentIter != parents.end();
         ++parentIter
         ) {
        out << childPfx << "parent context ";
        (*parentIter)->dump(out, childPfx);
    }
    
    for (VarDefMap::const_iterator varIter = defs.begin();
         varIter != defs.end();
         ++varIter
         )
        varIter->second->dump(out, childPfx);
    out << prefix << "}\n";
}
