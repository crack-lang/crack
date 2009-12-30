
#include "TypeDef.h"

#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "AllocExpr.h"
#include "AssignExpr.h"
#include "ArgDef.h"
#include "Context.h"
#include "FuncDef.h"
#include "InstVarDef.h"
#include "OverloadDef.h"
#include "ResultExpr.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace model;
using namespace spug;

bool TypeDef::hasInstSlot() {
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

bool TypeDef::emitNarrower(TypeDef &target) {
    assert(context);
    
    // if the types are the same, we're done.
    if (this == &target)
        return true;
    
    // look through the parents
    int i = 0;
    for (Context::ContextVec::iterator iter = context->parents.begin();
         iter != context->parents.end();
         ++iter, ++i
         ) {
        assert((*iter)->returnType && 
               "Parent context is not bound to a type."
               );
        TypeDef &baseType = *(*iter)->returnType;
        if (baseType.emitNarrower(target)) {
            // have the builder actually emit tha narrower code to obtain an 
            // expression of the base type from this type.
            context->builder.emitNarrower(*this, baseType, i);
            return true;
        }
    }

    // the target type was not found in our ancestry.
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
                                                        "init",
                                                        voidType,
                                                        args
                                                        );

    // XXX do initialization for the base classes.

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
                throw Exception(SPUG_FSTR("no initializer for variable " << 
                                          ivar->name << 
                                          " while creating default "
                                          "constructor."
                                         )
                                );

            AssignExprPtr assign = new AssignExpr(thisRef.get(),
                                                  ivar,
                                                  ivar->initializer.get()
                                                  );
            context->builder.emitFieldAssign(*funcContext, assign.get());
        }
    
    context->builder.emitReturn(*funcContext, 0);
    context->builder.emitEndFunc(*funcContext, newFunc.get());
    context->addDef(newFunc.get());
    return newFunc;
}

void TypeDef::createNewFunc(FuncDef *initFunc) {
    ContextPtr funcContext = context->createSubContext(Context::local);
    
    // copy the original arg list
    FuncDef::ArgVec args(initFunc->args.size());
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
                                                        args
                                                        );
    // create "Type this = alloc(Type);"
    ExprPtr allocExpr = new AllocExpr(this);
    VarDefPtr thisVar = context->builder.emitVarDef(*funcContext, this,
                                                    "this",
                                                    allocExpr.get(),
                                                    false
                                                    );
    VarRefPtr thisRef = new VarRef(thisVar.get());

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
    VarDefPtr initMethods = context->lookUp("init", false);
    OverloadDef *overloads = OverloadDefPtr::rcast(initMethods);
    FuncDef *funcDef;
    if (overloads) {
        // multiple init funcs: create new functions for all of them.
        for (OverloadDef::FuncList::iterator iter = overloads->funcs.begin();
             iter != overloads->funcs.end();
             ++iter
             )
            createNewFunc(iter->get());
    } else if (funcDef = FuncDefPtr::rcast(initMethods)) {
        createNewFunc(funcDef);
    } else {
        // create a default constructor and wrap it in a new function.
        createNewFunc(createDefaultInit().get());
    }
}

FuncDefPtr TypeDef::getConverter(const TypeDef &other) {
    // XXX since we don't have a generic conversion framework in place yet, 
    // just do this for bool and voidptr.  What we want to do here is look up 
    // "oper to canonical-type-name"
    if (other.name == "bool") {
        FuncCall::ExprVec args;
        return context->lookUp("toBool", args);
    } else if (other.name == "voidptr") {
        FuncCall::ExprVec args;
        return context->lookUp("oper to voidptr", args);        
    } else {
        return 0;
    }
}
