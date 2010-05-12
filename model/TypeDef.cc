// Copyright 2009 Google Inc.

#include "TypeDef.h"

#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "parser/ParseError.h"  // need to move error handling into Context
#include "AllocExpr.h"
#include "AssignExpr.h"
#include "CleanupFrame.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "Initializers.h"
#include "InstVarDef.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace std;
using namespace model;
using namespace spug;
using parser::ParseError;

TypeDef *TypeDef::findSpecialization(TypeVec *types) {
    assert(generic && "find specialization called on non-generic type");
    SpecializationCache::iterator match = generic->find(types);
    if (match != generic->end() && match->first.equals(types))
        return match->second.get();
    else
        return 0;
}

bool TypeDef::hasInstSlot() {
    return false;
}

bool TypeDef::isImplicitFinal(const std::string &name) {
    return name == "oper init" ||
           name == "oper bind" ||
           name == "oper release" ||
           name == "toBool";
}

bool TypeDef::isDerivedFrom(const TypeDef *other) const {
    if (this == other)
        return true;

    Context::ContextVec &parents = context->parents;
    for (Context::ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if ((*iter)->returnType->isDerivedFrom(other))
            return true;
    return false;
}

VarDefPtr TypeDef::emitVarDef(Context &container, const std::string &name,
                              Expr *initializer
                              ) {
    return container.builder.emitVarDef(container, this, name, initializer);
}

bool TypeDef::matches(const TypeDef &other) const {
    if (&other == this)
        return true;
    else if (!other.context)
        // primitive types may not have contexts and hence may not have 
        // parents.
        return false;
    
    // try the parents
    Context::ContextVec &otherParents = other.context->parents;
    for (Context::ContextVec::iterator iter = otherParents.begin();
         iter != otherParents.end();
         ++iter
         ) {
        assert((*iter)->returnType && "type has a parent that is not a class");
        if (matches(*(*iter)->returnType))
            return true;
    }
    
    return false;
}    

FuncDefPtr TypeDef::createDefaultInit() {
    ContextPtr funcContext = context->createSubContext(Context::local);

    // create the "this" variable
    ArgDefPtr thisDef = context->builder.createArgDef(this, "this");
    funcContext->addDef(thisDef.get());
    VarRefPtr thisRef = new VarRef(thisDef.get());
    
    FuncDef::ArgVec args(0);
    TypeDef *voidType = context->globalData->voidType.get();
    FuncDefPtr newFunc = context->builder.emitBeginFunc(*funcContext,
                                                        FuncDef::method,
                                                        "oper init",
                                                        voidType,
                                                        args,
                                                        0
                                                        );

    // do initialization for the base classes.
    for (Context::ContextVec::iterator ibase = context->parents.begin();
         ibase != context->parents.end();
         ++ibase
         ) {

        // if the base class contains no constructors at all, either it's a 
        // special class or it has no need for constructors, so ignore it.
        OverloadDefPtr overloads = (*ibase)->lookUp("oper init");
        if (!overloads)
            continue;

        // we must get a default initializer and it must be specific to the 
        // base class (not inherited from an ancestor of the base class)
        FuncDef::ArgVec args;
        FuncDefPtr baseInit = overloads->getSigMatch(args);
        if (!baseInit || baseInit->context != ibase->get())
            // XXX make this a parser error
            throw ParseError(SPUG_FSTR("Cannot create a default constructor "
                                        "because base class " << 
                                        (*ibase)->returnType->name <<
                                        " has no default constructor."
                                       )
                             );

        FuncCallPtr funcCall = context->builder.createFuncCall(baseInit.get());
        funcCall->receiver = thisRef;
        funcCall->emit(*context);
    }

    // generate constructors for all of the instance variables
    for (Context::VarDefMap::iterator iter = context->beginDefs();
         iter != context->endDefs();
         ++iter
         )
        if (iter->second->hasInstSlot()) {
            InstVarDef *ivar = InstVarDefPtr::arcast(iter->second);
            
            // when creating a default constructor, everything has to have an
            // initializer.
            // XXX make this a parser error
            if (!ivar->initializer)
                throw ParseError(SPUG_FSTR("no initializer for variable " << 
                                           ivar->name << 
                                           " while creating default "
                                           "constructor."
                                          )
                                 );

            AssignExprPtr assign = new AssignExpr(thisRef.get(),
                                                  ivar,
                                                  ivar->initializer.get()
                                                  );
            context->builder.emitFieldAssign(*funcContext, thisRef.get(),
                                             assign.get()
                                             );
        }
    
    context->builder.emitReturn(*funcContext, 0);
    context->builder.emitEndFunc(*funcContext, newFunc.get());
    context->addDef(newFunc.get());
    return newFunc;
}

void TypeDef::createDefaultDestructor() {
    ContextPtr funcContext = context->createSubContext(Context::local);

    // create the "this" variable
    ArgDefPtr thisDef = context->builder.createArgDef(this, "this");
    funcContext->addDef(thisDef.get());
    
    FuncDef::Flags flags = 
        FuncDef::method | 
        (hasVTable ? FuncDef::virtualized : FuncDef::noFlags);
    
    FuncDef::ArgVec args(0);
    TypeDef *voidType = context->globalData->voidType.get();
    FuncDefPtr delFunc = context->builder.emitBeginFunc(*funcContext,
                                                        flags,
                                                        "oper del",
                                                        voidType,
                                                        args,
                                                        0
                                                        );

    // all we have to do is add the destructor cleanups
    addDestructorCleanups(*funcContext);

    // ... and close off the function
    context->builder.emitReturn(*funcContext, 0);
    context->builder.emitEndFunc(*funcContext, delFunc.get());
    context->addDef(delFunc.get());
}

void TypeDef::createNewFunc(FuncDef *initFunc) {
    ContextPtr funcContext = context->createSubContext(Context::local);
    
    // copy the original arg list
    FuncDef::ArgVec args;
    for (FuncDef::ArgVec::iterator iter = initFunc->args.begin();
         iter != initFunc->args.end();
         ++iter
         ) {
        ArgDefPtr argDef =
            context->builder.createArgDef((*iter)->type.get(), 
                                          (*iter)->name
                                          );
        args.push_back(argDef);
        funcContext->addDef(argDef.get());
    }
    
    FuncDefPtr newFunc = context->builder.emitBeginFunc(*funcContext, 
                                                        FuncDef::noFlags,
                                                        "oper new",
                                                        this,
                                                        args,
                                                        0
                                                        );
    // create "Type this = alloc(Type);"
    ExprPtr allocExpr = new AllocExpr(this);
    VarDefPtr thisVar = context->builder.emitVarDef(*funcContext, this,
                                                    "this",
                                                    allocExpr.get(),
                                                    false
                                                    );
    VarRefPtr thisRef = new VarRef(thisVar.get());
    
    // initialize all vtable_base pointers. XXX hack.  Replace this with code 
    // in vtable_base.oper init() once we get proper constructor composition
    if (hasVTable) {
        thisRef->emit(*funcContext);
        context->builder.emitVTableInit(*funcContext, this);
    }

    // create "this.init(*args);"
    FuncCallPtr initFuncCall = new FuncCall(initFunc);
    FuncCall::ExprVec initArgs(args.size());
    for (FuncDef::ArgVec::iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        initFuncCall->args.push_back(new VarRef(iter->get()));
    initFuncCall->receiver = thisRef;
    initFuncCall->emit(*funcContext);
    
    // return the resulting object and close the new function
    context->builder.emitReturn(*funcContext, thisRef.get());
    context->builder.emitEndFunc(*funcContext, newFunc.get());

    // register it in the class
    context->addDef(newFunc.get());

    // if this is the default initializer, store a call to it
    if (initFunc->args.size() == 0)
        defaultInitializer = new FuncCall(newFunc.get());
}

void TypeDef::rectify() {
    // clear the default initializer
    defaultInitializer = 0;
    
    // collect all of the init methods.
    VarDefPtr initMethods = context->lookUp("oper init", false);
    OverloadDef *overloads = OverloadDefPtr::rcast(initMethods);
    FuncDef *funcDef;
    bool gotInit = false;
    if (overloads) {
        // multiple init funcs: create new functions for all of them.
        for (OverloadDef::FuncList::iterator iter = overloads->beginTopFuncs();
             iter != overloads->endTopFuncs();
             ++iter
             ) {
            createNewFunc(iter->get());
            gotInit = true;
        }
    } else if (funcDef = FuncDefPtr::rcast(initMethods)) {
        // all functions should now be wrapped in overloads
        assert(false && "got plain func def for init");
    }
    
    // if there are no init functions specific to this class, create a 
    // default constructor and wrap it in a new function.
    if (!gotInit)
        createNewFunc(createDefaultInit().get());
    
    // if the class doesn't already define a delete operator specific to the 
    // class, generate one.
    FuncDefPtr operDel = context->lookUpNoArgs("oper del");
    if (!operDel || operDel->context != context.get())
        createDefaultDestructor();
}

bool TypeDef::isParent(TypeDef *type) {
    Context::ContextVec &parents = context->parents;
    for (Context::ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (type == (*iter)->returnType.get())
            return true;
    
    return false;
}

FuncDefPtr TypeDef::getConverter(const TypeDef &other) {
    // XXX This is a half-assed general solution to the problem, we should 
    // really be using the canonical name of the type (and omitting the 
    // special case for bool).
    if (other.name == "bool") {
        return context->lookUpNoArgs("toBool");
    } else {
        return context->lookUpNoArgs("oper to " + other.name);
    }
}

bool TypeDef::getPathToAncestor(const TypeDef &ancestor, 
                                TypeDef::AncestorPath &path,
                                unsigned depth
                                ) {
    if (this == &ancestor) {
        path.resize(depth);
        return true;
    }
        
    int i = 0;
    Context::ContextVec &parents = context->parents;
    for (Context::ContextVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter, ++i
         ) {
        TypeDef *base = (*iter)->returnType.get();
        if (base->getPathToAncestor(ancestor, path, depth + 1)) {
            path[depth].index = i;
            path[depth].ancestor = base;
            return true;
        }
    }
    
    return false;
}

void TypeDef::emitInitializers(Context &context, Initializers *inits) {

    VarDefPtr thisDef = context.lookUp("this");
    assert(thisDef && 
            "trying to emit initializers in a context with no 'this'");
    VarRefPtr thisRef = new VarRef(thisDef.get());

    // do initialization for the base classes.
    ContextPtr classCtx = context.getClassContext();
    for (Context::ContextVec::iterator ibase = classCtx->parents.begin();
         ibase != classCtx->parents.end();
         ++ibase
         ) {
        TypeDef *base = (*ibase)->returnType.get();

        // see if there's a constructor for the base class in our list of 
        // initilaizers.
        FuncCallPtr initCall = inits->getBaseInitializer(base);
        if (initCall) {
            initCall->emit(context);
            continue;
        }

        // if the base class contains no constructors at all, either it's a 
        // special class or it has no need for constructors, so ignore it.
        OverloadDefPtr overloads = (*ibase)->lookUp("oper init");
        if (!overloads)
            continue;

        // we must get a default initializer and it must be specific to the 
        // base class (not inherited from an ancestor of the base class)
        FuncDef::ArgVec args;
        FuncDefPtr baseInit = overloads->getSigMatch(args);
        if (!baseInit || baseInit->context != ibase->get())
            // XXX make this a parser error
            throw ParseError(SPUG_FSTR("Cannot create a default constructor "
                                        "because base class " << 
                                        (*ibase)->returnType->name <<
                                        " has no default constructor."
                                       )
                             );

        FuncCallPtr funcCall = context.builder.createFuncCall(baseInit.get());
        funcCall->receiver = thisRef;
        funcCall->emit(context);
    }

    // generate constructors for all of the instance variables
    for (Context::VarDefMap::iterator iter = classCtx->beginDefs();
         iter != classCtx->endDefs();
         ++iter
         )
        // XXX need to put these in order of definition
        if (iter->second->hasInstSlot()) {
            InstVarDef *ivar = InstVarDefPtr::arcast(iter->second);
            
            // see if the user has supplied an initializer, use it if so.
            ExprPtr initializer = inits->getFieldInitializer(ivar);
            if (!initializer)
                initializer = ivar->initializer;
            
            // when creating a default constructor, everything has to have an
            // initializer.
            // XXX make this a parser error
            if (!initializer)
                throw ParseError(SPUG_FSTR("no initializer for variable " << 
                                           ivar->name << 
                                           " while creating default "
                                           "constructor."
                                          )
                                 );

            AssignExprPtr assign = new AssignExpr(thisRef.get(),
                                                  ivar,
                                                  initializer.get()
                                                  );
            context.builder.emitFieldAssign(context, thisRef.get(),
                                            assign.get()
                                            );
        }
    
    initializersEmitted = true;
}

void TypeDef::addDestructorCleanups(Context &context) {
    ContextPtr classCtx = context.getClassContext();
    VarRefPtr thisRef = new VarRef(context.lookUp("this").get());
    
    // first add the cleanups for the base classes, in order defined, then the 
    // cleanups for the derived classes.  Cleanups are applied in the reverse 
    // order that they are added, so this will result in the expected 
    // destruction order of instance variables followed by base classes.
    
    // generate calls to the destructors for all of the base classes.
    for (Context::ContextVec::iterator ibase = classCtx->parents.begin();
         ibase != classCtx->parents.end();
         ++ibase
         ) {
        TypeDefPtr base = (*ibase)->returnType;
        
        // check for a delete operator (the primitive base classes don't have 
        // them and don't need cleanup)
        FuncDefPtr operDel = (*ibase)->lookUpNoArgs("oper del");
        if (!operDel)
            continue;
        
        // create a cleanup function and don't call it through the vtable.
        FuncCallPtr funcCall =
            context.builder.createFuncCall(operDel.get(), true);

        funcCall->receiver = thisRef.get();
        context.cleanupFrame->addCleanup(funcCall.get());
    }

    // generate destructors for all of the instance variables
    // XXX again, need to do this in order of definition
    for (Context::VarDefMap::iterator iter = classCtx->beginDefs();
         iter != classCtx->endDefs();
         ++iter
         )
        if (iter->second->hasInstSlot()) {
            context.cleanupFrame->addCleanup(iter->second.get(), 
                                             thisRef.get()
                                             );
        }
    
    initializersEmitted = true;
}

TypeDef *TypeDef::getSpecialization(Context &context, 
                                    TypeDef::TypeVec *types
                                    ) {
    assert(false && "generics are not yet supported for normal types.");
}

void TypeDef::dump(ostream &out, const string &prefix) const {
    out << prefix << "class " << getFullName() << " {" << endl;
    string childPrefix = prefix + "  ";
    for (Context::VarDefMap::iterator iter = context->beginDefs();
         iter != context->endDefs();
         ++iter
         )
        iter->second->dump(out, childPrefix);
    out << prefix << "}" << endl;
}
