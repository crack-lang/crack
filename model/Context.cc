// Copyright 2009 Google Inc.

#include "Context.h"

#include <stdlib.h>
#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "parser/Token.h"
#include "parser/Location.h"
#include "parser/ParseError.h"
#include "Annotation.h"
#include "AssignExpr.h"
#include "BuilderContextData.h"
#include "CleanupFrame.h"
#include "ConstVarDef.h"
#include "FuncAnnotation.h"
#include "ArgDef.h"
#include "Branchpoint.h"
#include "GlobalNamespace.h"
#include "IntConst.h"
#include "LocalNamespace.h"
#include "ModuleDef.h"
#include "NullConst.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "StrConst.h"
#include "TernaryExpr.h"
#include "TypeDef.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace std;

parser::Location Context::emptyLoc;

void Context::warnOnHide(const string &name) {
    if (ns->lookUp(name))
        cerr << loc.getName() << ":" << loc.getLineNumber() << ": " << 
            "Symbol " << name << 
            " hides another definition in an enclosing context." << endl;
}

namespace {
    void collectAncestorOverloads(OverloadDef *overload,
                                  Namespace *srcNs
                                  ) {
        NamespacePtr parent;
        for (unsigned i = 0; parent = srcNs->getParent(i++);) {
            VarDefPtr var = parent->lookUp(overload->name, false);
            OverloadDefPtr parentOvld;
            if (!var) {
                // the parent does not have this overload.  Check the next level.
                collectAncestorOverloads(overload, parent.get());
            } else {
                parentOvld = OverloadDefPtr::rcast(var);
                // if there is a variable of this name but it is not an overload, 
                // we have a situation where there is a non-overload definition in 
                // an ancestor namespace that will block resolution of the 
                // overloads in all derived namespaces.  This is a bad thing, 
                // but not something we want to deal with here.

                if (parentOvld)
                    overload->addParent(parentOvld.get());
            }
        }
    }
}

OverloadDefPtr Context::replicateOverload(const std::string &varName,
                                          Namespace *srcNs
                                          ) {
    OverloadDefPtr overload = new OverloadDef(varName);
    overload->type = construct->overloadType;
    
    // merge in the overloads from the parents
    collectAncestorOverloads(overload.get(), srcNs);
    srcNs->addDef(overload.get());
    return overload;
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext,
                 Namespace *ns,
                 Namespace *compileNS
                 ) :
    loc(parentContext ? parentContext->loc : emptyLoc),
    parent(parentContext),
    ns(ns),
    compileNS(compileNS ? compileNS : 
                (parentContext ? parentContext->compileNS : NamespacePtr(0))),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    nextFuncFlags(FuncDef::noFlags),
    construct(parentContext->construct),
    cleanupFrame(builder.createCleanupFrame(*this)) {
    assert(construct && "parent context must have a construct");
}

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Construct *construct,
                 Namespace *ns,
                 Namespace *compileNS
                 ) :
    ns(ns),
    compileNS(compileNS),
    builder(builder),
    scope(scope),
    toplevel(false),
    emittingCleanups(false),
    terminal(false),
    returnType(TypeDefPtr(0)),
    nextFuncFlags(FuncDef::noFlags),
    construct(construct),
    cleanupFrame(builder.createCleanupFrame(*this)) {
}

Context::~Context() {}

ContextPtr Context::createSubContext(Scope newScope, Namespace *ns,
                                     const string *name
                                     ) {
    if (!ns) {
        switch (newScope) {
            case local:
                ns = new LocalNamespace(this->ns.get(), name ? *name : "");
                break;
            case module:
                ns = new GlobalNamespace(this->ns.get(), name ? *name : "");
                break;
            case composite:
            case instance:
                assert(false && 
                        "missing namespace when creating composite or instance "
                        "context"
                       );
        }
    }
    return new Context(builder, newScope, this, ns, compileNS.get());
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

ModuleDefPtr Context::createModule(const string &name) {
    return builder.createModule(*this, name);
}

ExprPtr Context::getStrConst(const std::string &value, bool raw) {
    
    // look up the raw string constant
    StrConstPtr strConst;
    Construct::StrConstTable::iterator iter = 
        construct->strConstTable.find(value);
    if (iter != construct->strConstTable.end()) {
        strConst = iter->second;
    } else {
        // create a new one
        strConst = builder.createStrConst(*this, value);
        construct->strConstTable[value] = strConst;
    }
    
    // if we don't have a StaticString type yet (or the caller wants a raw
    // bytestr), we're done.
    if (raw || !construct->staticStringType)
        return strConst;
    
    // create the "new" expression for the string.
    vector<ExprPtr> args;
    args.push_back(strConst);
    args.push_back(builder.createIntConst(*this, value.size(),
                                          construct->uintType.get()
                                          )
                   );
    FuncDefPtr newFunc =
        lookUp("oper new", args, construct->staticStringType.get());
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

VarDefPtr Context::emitVarDef(Context *defCtx, TypeDef *type,
                              const std::string &name,
                              Expr *initializer
                              ) {
    createCleanupFrame();
    VarDefPtr varDef = type->emitVarDef(*this, name, initializer);
    closeCleanupFrame();
    defCtx->ns->addDef(varDef.get());
    cleanupFrame->addCleanup(varDef.get());
    return varDef;
}

void Context::emitVarDef(TypeDef *type, const parser::Token &tok, 
                         Expr *initializer
                         ) {

    if (construct->migrationWarnings) {
        if (initializer && NullConstPtr::cast(initializer)) {
            cerr << loc.getName() << ":" << loc.getLineNumber() << ": " <<
                "unnecessary initialization to null" << endl;
        } else if (!initializer && type->pointer) {
            cerr << loc.getName() << ":" << loc.getLineNumber() << ": " <<
                "default initializer is now null!" << endl;
        }
    }

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

    emitVarDef(defCtx.get(), type, tok.getData(), initializer);
}

ExprPtr Context::createTernary(Expr *cond, Expr *trueVal, Expr *falseVal) {
    // make sure the condition can be converted to bool
    ExprPtr boolCond = cond->convert(*this, construct->boolType.get());
    if (!boolCond)
        error("Condition in ternary operator is not boolean.");

    ExprPtr converted;

    // make sure the types are compatible
    TypeDefPtr type;
    if (trueVal->type != falseVal->type) {
        if (trueVal->type->isDerivedFrom(falseVal->type.get())) {
            type = falseVal->type;
        } else if (falseVal->type->isDerivedFrom(trueVal->type.get())) {
            type = trueVal->type;
        } else if (converted = falseVal->convert(*this, trueVal->type.get())) {
            type = trueVal->type;
            falseVal = converted.get();
        } else if (converted = trueVal->convert(*this, falseVal->type.get())) {
            type = falseVal->type;
            trueVal = converted.get();
        } else {
            error("Value types in ternary operator are not compatible.");
        }
    } else {
        // the types are equal
        type = trueVal->type;
    }
    
    return builder.createTernary(*this, boolCond.get(), trueVal, falseVal, 
                                 type.get()
                                 );
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
    

ExprPtr Context::createVarRef(VarDef *varDef) {

    // is the variable a constant?
    ConstVarDefPtr constDef;
    if (constDef = ConstVarDefPtr::cast(varDef))
        return constDef->expr;
    
    // verify that the variable is reachable
    
    // if the variable is in a module context, it is accessible
    if (ModuleDefPtr::cast(varDef->getOwner()) ||
        GlobalNamespacePtr::cast(varDef->getOwner())) {
        return builder.createVarRef(varDef);
    
    // if it's in an instance context, verify that this is either the composite
    // context of the class or a method of the class that contains the variable.
    } else if (TypeDefPtr::cast(varDef->getOwner())) {
        if (scope == composite && varDef->getOwner() == parent->ns ||
            getToplevel()->parent->ns
            ) {
            return builder.createVarRef(varDef);
        }
    
    // if it's in a function context, make sure it's this function
    } else if (inSameFunc(varDef->getOwner())) {
        return builder.createVarRef(varDef);
    }
    
    error(SPUG_FSTR("Variable '" << varDef->name << 
                     "' is not accessible from within this context."
                    )
          );
}

VarRefPtr Context::createFieldRef(Expr *aggregate, VarDef *var) {
    TypeDef *aggType = aggregate->type.get();
    TypeDef *varNS = TypeDefPtr::cast(var->getOwner());
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

void Context::setCatchBranchpoint(Branchpoint *branch) {
    catchBranch = branch;
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

ContextPtr Context::getCatch() {
    if (catchBranch)
        return this;
    else if (toplevel)
        return parent;
    else if (parent)
        return parent->getCatch();
}

BranchpointPtr Context::getCatchBranchpoint() {
    return catchBranch;
}

ExprPtr Context::makeThisRef(const string &memberName) {
   VarDefPtr thisVar = ns->lookUp("this");
   if (!thisVar)
      error(SPUG_FSTR("instance member " << memberName <<
                       " may not be used in a static context."
                      )
            );
          
   return createVarRef(thisVar.get());
}

void Context::expandIteration(const std::string &name, bool defineVar,
                              bool isIter,
                              Expr *seqExpr,
                              ExprPtr &cond,
                              ExprPtr &beforeBody,
                              ExprPtr &afterBody
                              ) {
    // verify that the sequence has an "iter" method
    FuncDefPtr iterFunc = lookUpNoArgs("iter", true, seqExpr->type.get());
    if (!iterFunc)
        error("iteration expression has no 'iter' method.");
    
    // create an expression from it
    FuncCallPtr iterCall = builder.createFuncCall(iterFunc.get());
    if (iterFunc->flags & FuncDef::method)
        iterCall->receiver = seqExpr;
    
    // if the variable provided is an iterator, just use it and clear the 
    // "var" argument as an indicator to later code.
    VarDefPtr iterVar, var;
    FuncDefPtr elemFunc;
    if (isIter) {
        // this is a "for on" and the variable is an iterator.
        if (defineVar) {
            assert(scope != composite && 
                    "iteration expanded in a non-definition context"
                );
            warnOnHide(name);
            iterVar = 
                emitVarDef(this, iterCall->type.get(), name, iterCall.get());
        } else {
            iterVar = ns->lookUp(name);
            if (iterVar->isConstant())
                error("Cannot use a constant as a loop iterator");
            if (!iterVar->type->matches(*iterCall->type))
                error("Loop iterator variable type does not match the type of "
                       "the iterator."
                      );

            // emit code to assign the iterator
            createCleanupFrame();
            ExprPtr iterAssign =
                AssignExpr::create(*this, iterVar.get(), iterCall.get());
            iterAssign->emit(*this)->handleTransient(*this);
            closeCleanupFrame();
        }
    } else {
        // we're passing in "this" and assuming that the current context is a 
        // definition context.
        assert(scope != composite && 
                "iteration expanded in a non-definition context"
               );
        iterVar = emitVarDef(this, iterCall->type.get(), ":iter", 
                             iterCall.get()
                             );

        elemFunc = lookUpNoArgs("elem", true, iterCall->type.get());
        
        if (defineVar) {
            warnOnHide(name);
            
            // if the element type doesn't have a default initializer, we need 
            // to create a null identifier for it so we don't seg-fault.
            ExprPtr initializer;
            if (!elemFunc->returnType->defaultInitializer)
                initializer = new NullConst(elemFunc->returnType.get());

            var = emitVarDef(this, elemFunc->returnType.get(), name, 
                             initializer.get()
                             );
        } else {
            var = ns->lookUp(name);
            if (var->isConstant())
                error("Cannot use a constant as a loop variable");
            if (!var->type->matches(*elemFunc->returnType))
                error("Loop variable type does not match the type of the "
                       "return value of the iterator's elem() method."
                      );
        }
    }
    
    // create a reference expression for the iterator
    ExprPtr iterRef = createVarRef(iterVar.get());
    
    if (var) {
        // assign the variable before the body
        FuncCallPtr elemCall = builder.createFuncCall(elemFunc.get());
        if (elemFunc->flags & FuncDef::method)
            elemCall->receiver = iterRef;
        beforeBody = AssignExpr::create(*this, var.get(), elemCall.get());
    }
    
    // convert it to a boolean for the condition
    cond = iterRef->convert(*this, construct->boolType.get());
    if (!cond)
        error("The iterator in a 'for' loop must convert to boolean.");
    
    // create the "iter.next()" expression
    FuncDefPtr nextFunc = lookUpNoArgs("next", true, iterRef->type.get());
    if (!nextFunc)
        error("The iterator in a 'for' loop must provide a 'next()' method");
    FuncCallPtr nextCall = builder.createFuncCall(nextFunc.get());
    if (nextFunc->flags & FuncDef::method)
        nextCall->receiver = iterRef;
    afterBody = nextCall;
}

VarDefPtr Context::lookUp(const std::string &varName, Namespace *srcNs) {
    if (!srcNs)
        srcNs = ns.get();
    VarDefPtr def = srcNs->lookUp(varName);

    // if we got an overload, we may need to create an overload in this
    // context.  (we can get away with checking the owner because overloads 
    // are never aliased)
    OverloadDef *overload = OverloadDefPtr::rcast(def);
    if (overload && overload->getOwner() != srcNs)
        return replicateOverload(varName, srcNs);
    else
        return def;
}

FuncDefPtr Context::lookUp(const std::string &varName,
                           vector<ExprPtr> &args,
                           Namespace *srcNs
                           ) {
    if (!srcNs)
        srcNs = ns.get();

    // do a lookup, if nothing was found no further action is necessary.
    VarDefPtr var = lookUp(varName, srcNs);
    if (!var)
        return 0;
    
    // if "var" is a class definition, convert this to a lookup of the "oper 
    // new" function on the class.
    TypeDef *typeDef = TypeDefPtr::rcast(var);
    if (typeDef) {
        FuncDefPtr operNew = lookUp("oper new", args, typeDef);

        // make sure we got it, and we didn't inherit it
        if (!operNew || operNew->getOwner() != typeDef)
            return 0;
        
        return operNew;
    }
    
    // make sure we got an overload
    OverloadDefPtr overload = OverloadDefPtr::rcast(var);
    if (!overload)
        return 0;
    
    // look up the signature in the overload
    return overload->getMatch(*this, args);
}

FuncDefPtr Context::lookUpNoArgs(const std::string &name, bool acceptAlias,
                                 Namespace *srcNs
                                 ) {
    OverloadDefPtr overload = OverloadDefPtr::rcast(lookUp(name, srcNs));
    if (!overload)
        return 0;

    // we can just check for a signature match here - cheaper and easier.
    FuncDef::ArgVec args;
    FuncDefPtr result = overload->getNoArgMatch(acceptAlias);
    return result;
}

VarDefPtr Context::addDef(VarDef *varDef, Namespace *srcNs) {
    if (!srcNs)
        srcNs = ns.get();
    FuncDef *funcDef = FuncDefPtr::cast(varDef);
    if (funcDef) {
        OverloadDefPtr overload = 
            OverloadDefPtr::rcast(lookUp(varDef->name, srcNs));
        if (!overload)
            overload = replicateOverload(varDef->name, srcNs);
        overload->addFunc(funcDef);
        funcDef->setOwner(srcNs);
        return overload;
    } else {
        srcNs->addDef(varDef);
        return varDef;
    }
}

void Context::insureOverloadPath(Context *ancestor, OverloadDef *overload) {
    // see if we define the overload, if not we're done.
    OverloadDefPtr localOverload =
        OverloadDefPtr::rcast(ns->lookUp(overload->name, false));
    if (!localOverload)
        return;

    // this code assumes that 'ancestor' must always be a direct ancestor of 
    // our namespace.  This isn't strictly essential, but it shouldn't be 
    // necessary to support the general case.  If we change this assumption, 
    // we'll get an assertion failure to warn us.

    // see if the overload is one of our overload's parents, if so we're done.
    if (localOverload->hasParent(overload))
        return;

    // verify that we are directly derived from the namespace.
    NamespacePtr parentNs;
    for (int i = 0; parentNs = ns->getParent(i++);)
        if (parent.get() == ancestor)
            break;
    assert(parent && "insureOverloadPath(): parent is not a direct parent.");
    
    localOverload->addParent(overload);
}

AnnotationPtr Context::lookUpAnnotation(const std::string &name) {
    VarDefPtr result = compileNS->lookUp(name);
    if (!result)
        return 0;

    // if we've already got an annotation, we're done.
    AnnotationPtr ann;
    if (ann = AnnotationPtr::rcast(result))
        return ann;

    // create the arg list for the signature of an annotation (we don't need 
    // the builder to create an ArgDef here because it's just for a signature 
    // match).
    FuncDef::ArgVec args(1);
    args[0] = new ArgDef(construct->crackContext.get(), "context");

    OverloadDef *ovld = OverloadDefPtr::rcast(result);
    if (ovld) {
        FuncDefPtr f = ovld->getSigMatch(args);
        if (!f)
            return 0;
        ann = new FuncAnnotation(f.get());
        compileNS->addDef(ann.get());
        return ann;
    }
    
    // XXX can't deal with variables yet
    return 0;
}

namespace {
    struct ContextStack {
        const list<string> &stack;
        ContextStack(const list<string> &stack) : stack(stack) {}
    };

    ostream &operator <<(ostream &out, ContextStack a) {
        for (list<string>::const_iterator iter = a.stack.begin(); 
             iter != a.stack.end();
             ++iter
             )
            out << "\n  " << *iter;
        return out;
    }
}

void Context::error(const parser::Location &loc, const string &msg, 
                    bool throwException
                    ) {
    
    list<string> &ec = construct->errorContexts;
    if (throwException)
        throw parser::ParseError(SPUG_FSTR(loc.getName() << ':' <<
                                           loc.getLineNumber() << ": " <<
                                           msg <<
                                           ContextStack(ec) <<
                                           endl
                                           )
                                );
    else {
        cerr << "ParseError: " << loc.getName() << ":" << 
            loc.getLineNumber() << ": " << msg << ContextStack(ec) << endl;
        exit(1);
    }
    
}

void Context::warn(const parser::Location &loc, const string &msg) {
    cerr << loc.getName() << ":" << loc.getLineNumber() << ": " << msg << endl;
}

void Context::pushErrorContext(const string &msg) {
    construct->errorContexts.push_front(msg);
}

void Context::popErrorContext() {
    construct->errorContexts.pop_front();
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
