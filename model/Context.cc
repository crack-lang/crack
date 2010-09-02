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
#include "GlobalNamespace.h"
#include "IntConst.h"
#include "LocalNamespace.h"
#include "ModuleDef.h"
#include "OverloadDef.h"
#include "StrConst.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;

parser::Location Context::emptyLoc;

Context::GlobalData::GlobalData() : 
    objectType(0), stringType(0), staticStringType(0) {
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext,
                 Namespace *ns
                 ) :
    loc(parentContext ? parentContext->loc : emptyLoc),
    parent(parentContext),
    ns(ns),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    globalData(parentContext ? parentContext->globalData : new GlobalData()),
    cleanupFrame(builder.createCleanupFrame(*this)) {
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context::GlobalData *globalData,
                 Namespace *ns
                 ) :
    ns(ns),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    returnType(TypeDefPtr(0)),
    globalData(globalData),
    cleanupFrame(builder.createCleanupFrame(*this)) {
}

Context::~Context() {}

ContextPtr Context::createSubContext(Scope newScope, Namespace *ns) {
    if (!ns) {
        switch (newScope) {
            case local:
                ns = new LocalNamespace(this->ns.get());
                break;
            case module:
                ns = new GlobalNamespace(this->ns.get());
                break;
            case composite:
            case instance:
                assert(false && 
                        "missing namespace when creating composite or instance "
                        "context"
                       );
        }
    }
    return new Context(builder, newScope, this, ns);
}

ContextPtr Context::getClassContext() {
    if (scope == instance)
        return this;
    else if (parent)
        return parent->getClassContext();
    else
        return 0;
}

ContextPtr Context::getDefContext() {
    if (scope != composite)
        return this;
    else if (parent)
        return parent->getDefContext();
    else
        return 0;
}

ContextPtr Context::getToplevel() {
    if (toplevel)
        return this;
    else if (parent)
        return parent->getToplevel();
    else
        return 0;
}

bool Context::encloses(const Context &other) const {
    if (this == &other)
        return true;
    else if (parent)
        return encloses(*parent);
    else
        return false;
}

ModuleDefPtr Context::createModule(const string &name,
                                   bool emitDebugInfo
                                   ) {
    return builder.createModule(*this, name, emitDebugInfo);
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
        globalData->staticStringType->lookUp(*this, "oper new", args);
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

TypeDefPtr Context::createForwardClass(const string &name) {
    TypeDefPtr type = builder.createClassForward(*this, name);
    ns->addDef(type.get());
    return type;
}

void Context::checkForUnresolvedForwards() {
    for (Namespace::VarDefMap::iterator iter = ns->beginDefs();
         iter != ns->endDefs();
         ++iter
         ) {
        
        // check for an overload
        OverloadDef *overload;
        if (overload = OverloadDefPtr::rcast(iter->second)) {
            for (OverloadDef::FuncList::iterator fi = 
                    overload->beginTopFuncs();
                 fi != overload->endTopFuncs();
                 ++fi
                 )
                if ((*fi)->flags & FuncDef::forward)
                    error(SPUG_FSTR("Forward declared function not defined at "
                                     "the end of the block: " << **fi
                                    )
                          );
        }
    }
}

void Context::emitVarDef(TypeDef *type, const parser::Token &tok, 
                         Expr *initializer
                         ) {

    // make sure we aren't using a forward declared type (we disallow this 
    // because we don't know if oper release() is defined for the type)
    if (type->forward)
        error(SPUG_FSTR("You cannot define a variable of a forward declared "
                         "type."
                        )
              );
    
    // if the definition context is an instance context, make sure that we 
    // haven't generated any constructors.
    ContextPtr defCtx = getDefContext();
    if (defCtx->scope == Context::instance && 
        TypeDefPtr::arcast(defCtx->ns)->initializersEmitted) {
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
    defCtx->ns->addDef(varDef.get());
    cleanupFrame->addCleanup(varDef.get());
}

bool Context::inSameFunc(Namespace *varNS) {
    if (scope != local)
        // this is not a function.
        return false;

    // see if the namespace is our namespace
    if (varNS == ns)
        return true;
        
    // if this isn't the toplevel context, check the parent
    else if (!toplevel)
        return parent->inSameFunc(varNS);
    else
        return false;
}
    

VarRefPtr Context::createVarRef(VarDef *varDef) {
    // verify that the variable is reachable
    
    // if the variable is in a module context, it is accessible
    if (ModuleDefPtr::cast(varDef->owner) || 
        GlobalNamespacePtr::cast(varDef->owner)) {
        return builder.createVarRef(varDef);
    
    // if it's in an instance context, verify that this is either the composite
    // context of the class or a method of the class that contains the variable.
    } else if (TypeDefPtr::cast(varDef->owner)) {
        if (scope == composite && varDef->owner == parent->ns ||
            getToplevel()->parent->ns
            ) {
            return builder.createVarRef(varDef);
        }
    
    // if it's in a function context, make sure it's this function
    } else if (inSameFunc(varDef->owner)) {
        return builder.createVarRef(varDef);
    }
    
    error(SPUG_FSTR("Variable '" << varDef->name << 
                     "' is not accessible from within this context."
                    )
          );
}

VarRefPtr Context::createFieldRef(Expr *aggregate, VarDef *var) {
    TypeDef *aggType = aggregate->type.get();
    TypeDef *varNS = TypeDefPtr::cast(var->owner);
    if (!varNS || !aggType->isDerivedFrom(varNS))
        error(SPUG_FSTR("Variable '" << var->name <<
                         "' is not accessible from within this context."
                        )
              );
    
    return builder.createFieldRef(aggregate, var);
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
    if (!toplevel && parent) {
        return parent->getBreak();
    } else {
        return 0;
    }
}

Branchpoint *Context::getContinue() {
    if (continueBranch)
        return continueBranch.get();

    // don't attempt to propagate out of an execution scope
    if (!toplevel && parent) {
        return parent->getContinue();
    } else {
        return 0;
    }
}

void Context::error(const string &msg) {
    throw parser::ParseError(SPUG_FSTR(loc.getName() << ':' <<
                                       loc.getLineNumber() << ": " <<
                                       msg
                                       )
                             );
}

void Context::dump(ostream &out, const std::string &prefix) const {
    switch (scope) {
        case module: out << "module "; break;
        case instance: out << "instance "; break;
        case local: out << "local "; break;
        case composite: out << "composite "; break;
        default: out << "UNKNOWN ";
    }
    ns->dump(out, prefix);
}

void Context::dump() {
    dump(cerr, "");
}
