// Copyright 2009 Google Inc.

#include "TypeDef.h"

#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include "builder/Builder.h"
#include "AllocExpr.h"
#include "AssignExpr.h"
#include "CleanupFrame.h"
#include "ArgDef.h"
#include "Branchpoint.h"
#include "Context.h"
#include "FuncDef.h"
#include "Initializers.h"
#include "InstVarDef.h"
#include "OverloadDef.h"
#include "NullConst.h"
#include "ResultExpr.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace std;
using namespace model;
using namespace spug;

TypeDef *TypeDef::findSpecialization(TypeVecObj *types) {
    assert(generic && "find specialization called on non-generic type");
    SpecializationCache::iterator match = generic->find(types);
    if (match != generic->end() && match->first.equals(types))
        return match->second.get();
    else
        return 0;
}

void TypeDef::storeDef(VarDef *def) {
    Namespace::storeDef(def);
    if (def->hasInstSlot())
        ordered.push_back(def);
}

NamespacePtr TypeDef::getParent(unsigned i) {
    if (i < parents.size())
        return parents[i];
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

    for (TypeVec::const_iterator iter = parents.begin();
           iter != parents.end();
           ++iter
           )
        if ((*iter)->isDerivedFrom(other))
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
    
    // try the parents
    for (TypeVec::const_iterator iter = other.parents.begin();
         iter != other.parents.end();
         ++iter
         )
        if (matches(**iter))
            return true;
    
    return false;
}    

FuncDefPtr TypeDef::createDefaultInit(Context &classContext) {
    ContextPtr funcContext = classContext.createSubContext(Context::local);

    // create the "this" variable
    ArgDefPtr thisDef = classContext.builder.createArgDef(this, "this");
    funcContext->ns->addDef(thisDef.get());
    VarRefPtr thisRef = new VarRef(thisDef.get());
    
    FuncDef::ArgVec args(0);
    TypeDef *voidType = classContext.globalData->voidType.get();
    FuncDefPtr newFunc = classContext.builder.emitBeginFunc(*funcContext,
                                                            FuncDef::method,
                                                            "oper init",
                                                            voidType,
                                                            args,
                                                            0
                                                            );

    // do initialization for the base classes.
    for (TypeVec::iterator ibase = parents.begin(); ibase != parents.end();
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
        if (!baseInit || baseInit->owner != ibase->get())
            classContext.error(SPUG_FSTR("Cannot create a default constructor "
                                          "because base class " << 
                                          (*ibase)->name <<
                                          " has no default constructor."
                                         )
                               );

        FuncCallPtr funcCall =
            classContext.builder.createFuncCall(baseInit.get());
        funcCall->receiver = thisRef;
        funcCall->emit(classContext);
    }

    // generate constructors for all of the instance variables in the order 
    // that they were declared.
    for (VarDefVec::iterator iter = beginOrderedDefs();
         iter != endOrderedDefs();
         ++iter
         ) {
        InstVarDef *ivar = InstVarDefPtr::arcast(*iter);
        
        // when creating a default constructor, everything has to have an
        // initializer.
        // XXX make this a parser error
        if (!ivar->initializer)
            classContext.error(SPUG_FSTR("no initializer for variable " << 
                                         ivar->name << 
                                         " while creating default "
                                         "constructor."
                                        )
                               );

        AssignExprPtr assign = new AssignExpr(thisRef.get(),
                                              ivar,
                                              ivar->initializer.get()
                                              );
        classContext.builder.emitFieldAssign(*funcContext, thisRef.get(),
                                             assign.get()
                                             );
        }
    
    classContext.builder.emitReturn(*funcContext, 0);
    classContext.builder.emitEndFunc(*funcContext, newFunc.get());
    addDef(newFunc.get());
    return newFunc;
}

void TypeDef::createDefaultDestructor(Context &classContext) {
    ContextPtr funcContext = classContext.createSubContext(Context::local);

    // create the "this" variable
    ArgDefPtr thisDef = classContext.builder.createArgDef(this, "this");
    funcContext->ns->addDef(thisDef.get());
    
    FuncDef::Flags flags = 
        FuncDef::method | 
        (hasVTable ? FuncDef::virtualized : FuncDef::noFlags);
    
    // check for an override
    FuncDefPtr override = lookUpNoArgs("oper del");
    
    FuncDef::ArgVec args(0);
    TypeDef *voidType = classContext.globalData->voidType.get();
    FuncDefPtr delFunc = classContext.builder.emitBeginFunc(*funcContext,
                                                            flags,
                                                            "oper del",
                                                            voidType,
                                                            args,
                                                            override.get()
                                                            );

    // all we have to do is add the destructor cleanups
    addDestructorCleanups(*funcContext);

    // ... and close off the function
    classContext.builder.emitReturn(*funcContext, 0);
    classContext.builder.emitEndFunc(*funcContext, delFunc.get());
    addDef(delFunc.get());
}

void TypeDef::createNewFunc(Context &classContext, FuncDef *initFunc) {
    ContextPtr funcContext = classContext.createSubContext(Context::local);
    funcContext->toplevel = true;
    funcContext->returnType = this;
    
    // copy the original arg list
    FuncDef::ArgVec args;
    for (FuncDef::ArgVec::iterator iter = initFunc->args.begin();
         iter != initFunc->args.end();
         ++iter
         ) {
        ArgDefPtr argDef =
            classContext.builder.createArgDef((*iter)->type.get(), 
                                              (*iter)->name
                                              );
        args.push_back(argDef);
        funcContext->ns->addDef(argDef.get());
    }
    
    FuncDefPtr newFunc = classContext.builder.emitBeginFunc(*funcContext, 
                                                            FuncDef::noFlags,
                                                            "oper new",
                                                            this,
                                                            args,
                                                            0
                                                            );
    // create "Type this = alloc(Type);"
    ExprPtr allocExpr = new AllocExpr(this);
    VarDefPtr thisVar = classContext.builder.emitVarDef(*funcContext, this,
                                                        "this",
                                                        allocExpr.get(),
                                                        false
                                                        );
    VarRefPtr thisRef = new VarRef(thisVar.get());
    
    // initialize all vtable_base pointers. XXX hack.  Replace this with code 
    // in vtable_base.oper init() once we get proper constructor composition
    if (hasVTable) {
        thisRef->emit(*funcContext);
        classContext.builder.emitVTableInit(*funcContext, this);
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
    classContext.builder.emitReturn(*funcContext, thisRef.get());
    classContext.builder.emitEndFunc(*funcContext, newFunc.get());

    // register it in the class
    addDef(newFunc.get());

    // if this is the default initializer, store a call to it
    if (initFunc->args.size() == 0)
        defaultInitializer = new FuncCall(newFunc.get());
}

void TypeDef::createCast(Context &outer) {
    assert(hasVTable && "Attempt to createCast() on a non-virtual class");
    ContextPtr funcCtx = outer.createSubContext(Context::local);
    funcCtx->toplevel = true;
    funcCtx->returnType = this;
    
    FuncDef::ArgVec args(1);
    args[0] = 
        outer.builder.createArgDef(outer.globalData->vtableBaseType.get(),
                                   "val"
                                   );
    FuncDefPtr castFunc = outer.builder.emitBeginFunc(*funcCtx,
                                                      FuncDef::noFlags,
                                                      "cast",
                                                      this,
                                                      args,
                                                      0
                                                      );
    
    // function body is:
    //  if (val.class.isSubclass(ThisClass);
    //      return ThisClass.unsafeCast(val);
    //  else
    //      __die("Invalid class cast");
    
    // val.class
    VarRefPtr valRef = funcCtx->builder.createVarRef(args[0].get());
    FuncDefPtr f = funcCtx->ns->lookUpNoArgs("oper class", false);
    assert(f && "oper class missing");
//  XXX this was trace code that mysteriously seg-faults: since I think there 
//  might be some memory corruption happening, I'm leaving this until I can 
//  investigate.
//    string s = f->getReceiverType()->name;
//    std::cerr << "Got oper class for " << s << endl;
    FuncCallPtr call = funcCtx->builder.createFuncCall(f.get());
    call->receiver = valRef;
    ExprPtr valClass = call;
    
    // $.isSubclass(ThisClass)
    FuncCall::ExprVec isSubclassArgs(1);
    isSubclassArgs[0] = funcCtx->builder.createVarRef(this);
    f = type->lookUp(*funcCtx, "isSubclass", isSubclassArgs);
    assert(f && "isSubclass missing");
    call = funcCtx->builder.createFuncCall(f.get());
    call->args = isSubclassArgs;
    call->receiver = valClass;

    // if ($)
    BranchpointPtr branchpoint = funcCtx->builder.emitIf(*funcCtx, call.get());
    
    // return ThisClass.unsafeCast(val);
    FuncCall::ExprVec unsafeCastArgs(1);
    unsafeCastArgs[0] = valRef;
    f = type->lookUp(*funcCtx, "unsafeCast", unsafeCastArgs);
    assert(f && "unsafeCast missing");
    call = funcCtx->builder.createFuncCall(f.get());
    call->args = unsafeCastArgs;
    funcCtx->builder.emitReturn(*funcCtx, call.get());

    // else    
    branchpoint = funcCtx->builder.emitElse(*funcCtx, branchpoint.get(), true);

    // __die() (we have to get it from the class's parent context)
    FuncCall::ExprVec abortArgs(1);
    abortArgs[0] = funcCtx->getStrConst("Invalid class cast.", true);
    f = outer.getParent()->ns->lookUp(*funcCtx, "__die", abortArgs);
    assert(f && "__die missing");
    call = funcCtx->builder.createFuncCall(f.get());
    call->args = abortArgs;
    funcCtx->createCleanupFrame();
    call->emit(*funcCtx)->handleTransient(*funcCtx);
    funcCtx->closeCleanupFrame();
    
    // need to "return null" to provide a terminator.
    TypeDef *vp = outer.globalData->voidPtrType.get();
    ExprPtr nullVal = (new NullConst(vp))->convert(*funcCtx, this);
    funcCtx->builder.emitReturn(*funcCtx, nullVal.get());

    // end of story.
    funcCtx->builder.emitEndIf(*funcCtx, branchpoint.get(), true);
    funcCtx->builder.emitEndFunc(*funcCtx, castFunc.get());
    
    // add the cast function to the meta-class
    type->addDef(castFunc.get());
}

void TypeDef::rectify(Context &classContext) {
    // if there are no init functions specific to this class, create a
    // default constructor and wrap it in a new function.
    if (!lookUp("oper init", false))
        createNewFunc(classContext, createDefaultInit(classContext).get());
    
    // if the class doesn't already define a delete operator specific to the 
    // class, generate one.
    FuncDefPtr operDel = lookUpNoArgs("oper del");
    if (!operDel || operDel->owner != this)
        createDefaultDestructor(classContext);
}

bool TypeDef::isParent(TypeDef *type) {
    for (TypeVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter
         )
        if (type == iter->get())
            return true;
    
    return false;
}

FuncDefPtr TypeDef::getConverter(const TypeDef &other) {
    // XXX This is a half-assed general solution to the problem, we should 
    // really be using the canonical name of the type (and omitting the 
    // special case for bool).
    if (other.name == "bool") {
        return lookUpNoArgs("toBool");
    } else {
        return lookUpNoArgs("oper to " + other.name);
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
    for (TypeVec::iterator iter = parents.begin();
         iter != parents.end();
         ++iter, ++i
         ) {
        TypeDef *base = iter->get();
        if (base->getPathToAncestor(ancestor, path, depth + 1)) {
            path[depth].index = i;
            path[depth].ancestor = base;
            return true;
        }
    }
    
    return false;
}

void TypeDef::emitInitializers(Context &context, Initializers *inits) {

    VarDefPtr thisDef = context.ns->lookUp("this");
    assert(thisDef && 
            "trying to emit initializers in a context with no 'this'");
    VarRefPtr thisRef = new VarRef(thisDef.get());

    // do initialization for the base classes.
    for (TypeVec::iterator ibase = parents.begin(); ibase != parents.end();
         ++ibase
         ) {
        TypeDef *base = ibase->get();

        // see if there's a constructor for the base class in our list of 
        // initilaizers.
        FuncCallPtr initCall = inits->getBaseInitializer(base);
        if (initCall) {
            initCall->emit(context);
            continue;
        }

        // if the base class contains no constructors at all, either it's a 
        // special class or it has no need for constructors, so ignore it.
        OverloadDefPtr overloads = base->lookUp("oper init");
        if (!overloads)
            continue;

        // we must get a default initializer and it must be specific to the 
        // base class (not inherited from an ancestor of the base class)
        FuncDef::ArgVec args;
        FuncDefPtr baseInit = overloads->getSigMatch(args);
        if (!baseInit || baseInit->owner != base)
            context.error(SPUG_FSTR("Cannot create a default constructor "
                                     "because base class " << 
                                     base->name <<
                                     " has no default constructor."
                                    )
                          );

        FuncCallPtr funcCall = context.builder.createFuncCall(baseInit.get());
        funcCall->receiver = thisRef;
        funcCall->emit(context);
    }

    // generate constructors for all of the instance variables
    for (VarDefVec::iterator iter = beginOrderedDefs();
         iter != endOrderedDefs();
         ++iter
         ) {
        InstVarDef *ivar = InstVarDefPtr::arcast(*iter);
        
        // see if the user has supplied an initializer, use it if so.
        ExprPtr initializer = inits->getFieldInitializer(ivar);
        if (!initializer)
            initializer = ivar->initializer;
        
        // when creating a default constructor, everything has to have an
        // initializer.
        // XXX make this a parser error
        if (!initializer)
            context.error(SPUG_FSTR("no initializer for variable " << 
                                     ivar->name << 
                                     " while creating default "
                                     "constructor."
                                    )
                           );
        
        // verify that we can convert the initializer to the type of the 
        // instance variable.
        ExprPtr converted = initializer->convert(context, ivar->type.get());
        if (!converted)
            context.error(SPUG_FSTR("Invalid type " << 
                                    initializer->type->name << 
                                    " for initializer for instance variable "
                                    << ivar->name << " of type " <<
                                    ivar->type->name
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
    VarRefPtr thisRef = new VarRef(context.ns->lookUp("this").get());
    
    // first add the cleanups for the base classes, in order defined, then the 
    // cleanups for the derived classes.  Cleanups are applied in the reverse 
    // order that they are added, so this will result in the expected 
    // destruction order of instance variables followed by base classes.
    
    // generate calls to the destructors for all of the base classes.
    for (TypeVec::iterator ibase = parents.begin();
         ibase != parents.end();
         ++ibase
         ) {
        TypeDef *base = ibase->get();
        
        // check for a delete operator (the primitive base classes don't have 
        // them and don't need cleanup)
        FuncDefPtr operDel = base->lookUpNoArgs("oper del");
        if (!operDel)
            continue;
        
        // create a cleanup function and don't call it through the vtable.
        FuncCallPtr funcCall =
            context.builder.createFuncCall(operDel.get(), true);

        funcCall->receiver = thisRef.get();
        context.cleanupFrame->addCleanup(funcCall.get());
    }

    // generate destructors for all of the instance variables in order of 
    // definition (cleanups run in the reverse order that they were added, 
    // which is exactly what we want).
    for (VarDefVec::iterator iter = beginOrderedDefs();
         iter != endOrderedDefs();
         ++iter
         )
        context.cleanupFrame->addCleanup(iter->get(), thisRef.get());
    
    initializersEmitted = true;
}

TypeDef *TypeDef::getSpecialization(Context &context, 
                                    TypeDef::TypeVecObj *types
                                    ) {
    assert(false && "generics are not yet supported for normal types.");
}

void TypeDef::dump(ostream &out, const string &prefix) const {
    out << prefix << "class " << getFullName() << " {" << endl;
    string childPrefix = prefix + "  ";
    
    for (TypeVec::const_iterator baseIter = parents.begin();
         baseIter != parents.end();
         ++baseIter
         ) {
        out << prefix << "parent" << endl;
        (*baseIter)->dump(out, prefix);
    }
    
    for (VarDefMap::const_iterator iter = beginDefs(); iter != endDefs();
         ++iter
         )
        iter->second->dump(out, childPrefix);
    out << prefix << "}" << endl;
}
