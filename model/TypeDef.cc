// Copyright 2009-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "TypeDef.h"

#include <spug/check.h>
#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include <spug/stlutil.h>
#include "builder/Builder.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "AllocExpr.h"
#include "AssignExpr.h"
#include "CleanupFrame.h"
#include "Deserializer.h"
#include "ArgDef.h"
#include "Branchpoint.h"
#include "Context.h"
#include "FuncDef.h"
#include "Generic.h"
#include "GlobalNamespace.h"
#include "Initializers.h"
#include "InstVarDef.h"
#include "OverloadDef.h"
#include "ModuleDef.h"
#include "NestedDeserializer.h"
#include "NullConst.h"
#include "ProtoBuf.h"
#include "ResultExpr.h"
#include "Serializer.h"
#include "VarDef.h"
#include "VarDefImpl.h"
#include "VarRef.h"

using namespace builder;
using namespace std;
using namespace model;
using namespace spug;
using namespace parser;

// returns true if func is non-null and abstract
bool TypeDef::isAbstract(FuncDef *func) {
    if (func && (func->flags & FuncDef::abstract)) {

        // found one.  do a look-up on the function, if there is a 
        // non-abstract implementation we should get a match that is 
        // _not_ abstract and has the same receiver type (to rule out the 
        // possibility that the implementation is for a method with the same 
        // signature in a different class).
        OverloadDefPtr overloads =
            OverloadDefPtr::rcast(lookUp(func->name));
        assert(overloads);
        FuncDefPtr nearest = overloads->getSigMatch(func->args);
        if (nearest->flags & FuncDef::abstract ||
            func->receiverType != nearest->receiverType
            )
            return true;
    }
    
    return false;
}

// if overload contains abstract functions, returns true and adds them to 
// abstractFuncs (assuming abstractFuncs is non-null)
bool TypeDef::hasAbstractFuncs(OverloadDef *overload,
                               vector<FuncDefPtr> *abstractFuncs
                               ) {
    bool gotAbstract = false;
    for (OverloadDef::FuncList::iterator iter = overload->beginTopFuncs();
         iter != overload->endTopFuncs();
         ++iter
         ) {
        if (isAbstract(iter->get()))
            if (abstractFuncs) {
                abstractFuncs->push_back(iter->get());
                gotAbstract = true;
            } else {
                return true;
            }
    }
    
    return gotAbstract;
}

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

TypeDef *TypeDef::extractInstantiation(ModuleDef *module, TypeVecObj *types) {
    TypeDefPtr result = module->getType(name);
    SPUG_CHECK(result, 
               "Instantiated generic " << module->getNamespaceName() <<
                " not defined in its module."
               );
    result->genericParms = *types;
    result->templateType = this;
    (*generic)[types] = result;
    return result.get();
}

ModuleDefPtr TypeDef::getModule() {
    return owner->getModule();
}

bool TypeDef::isHiddenScope() {
    return owner->isHiddenScope();
}

VarDef *TypeDef::asVarDef() {
    return this;
}

NamespacePtr TypeDef::getParent(unsigned i) {
    if (i < parents.size())
        return parents[i];
    else
        return 0;
}

NamespacePtr TypeDef::getNamespaceOwner() {
    return getOwner();
}

bool TypeDef::hasGenerics() const {
    return genericInfo || Namespace::hasGenerics();
}

FuncDefPtr TypeDef::getFuncDef(Context &context, 
                               std::vector<ExprPtr> &args
                               ) const {
    // Fixing "const" in lookup is a can of worms, so we just cast away const 
    // for now.  Lookups don't mutate.
    FuncDefPtr func = context.lookUp("oper new", args, 
                                     const_cast<TypeDef *>(this));

    if (!func) {
        if (abstract)
            context.error(
                SPUG_FSTR("You can not create an instance of abstract "
                           "class " << getDisplayName() << " without an "
                           "explicit 'oper new'."
                          )
            );
        else
            context.error(
                SPUG_FSTR("No constructor for " << name <<
                           " with these argument types: (" << args << 
                           ")"
                          )
            );
    } else if (func->returnType.get() != this) {
        if (abstract)
            context.error(
                SPUG_FSTR("You can not create an instance of abstract "
                           "class " << name << " without an "
                           "explicit 'oper new'."
                          )
            );
        else
            context.error(
                SPUG_FSTR("No constructor for " << name <<
                           " with these argument types: (" << args << 
                           ").  (Ancestor class " << 
                           TypeDefPtr::cast(func->getOwner())->getDisplayName()
                           << " has a matching constructor)"
                          )
            );
    }

    return func;
}

FuncDefPtr TypeDef::getOperNew(Context &context, 
                               std::vector<ExprPtr> &args
                               ) const {
    FuncDefPtr func = context.lookUp("oper new", args, 
                                     const_cast<TypeDef *>(this)
                                     );
    if (func && func->returnType.get() != this)
        func = 0;
    return func;
}

bool TypeDef::hasInstSlot() const {
    return false;
}

bool TypeDef::isImplicitFinal(const std::string &name) {
    return name == "oper init" ||
           name == "oper bind" ||
           name == "oper release" ||
           !name.compare(0, 2, "__");
}

void TypeDef::addToAncestors(Context &context, TypeVec &ancestors) {
    // ignore VTableBase
    if (this == context.construct->vtableBaseType)
        return;

    // make sure this isn't a primitive class (we use the "pointer" attribute 
    // to make this determination)
    if (!pointer)
        context.error(SPUG_FSTR("You may not inherit from " << 
                                getDisplayName() <<
                                 " because it's a primitive class."
                                )
                      );

    // store the current endpoint so we don't bother checking against our own 
    // ancestors.
    size_t initAncSize = ancestors.size();
    
    // if this is the object class, make sure that it's the first ancestor.
    if (initAncSize && this == context.construct->objectType)
        context.error("If you directly or indirectly inherit from Object, "
                       "Object (or its derivative) must come first in the "
                       "ancestor list.");

    for (TypeVec::const_iterator iter = parents.begin();
           iter != parents.end();
           ++iter
           )
        (*iter)->addToAncestors(context, ancestors);

    // make sure that we're not already in the ancestor list
    for (size_t i = 0; i < initAncSize; ++i)
        if (ancestors[i] == this)
            context.error(SPUG_FSTR("Class " << getDisplayName() <<
                                     " is already an ancestor."
                                    )
                          );

    // add this to the ancestors.
    ancestors.push_back(this);
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

FuncDefPtr TypeDef::createOperInit(Context &classContext, 
                                   const ArgVec &args
                                   ) {
    assert(classContext.ns.get() == this); // needed for final addDef()
    ContextPtr funcContext = classContext.createSubContext(Context::local);
    funcContext->toplevel = true;

    // create the "this" variable
    ArgDefPtr thisDef = classContext.builder.createArgDef(this, "this");
    funcContext->addDef(thisDef.get());
    VarRefPtr thisRef = new VarRef(thisDef.get());
    
    TypeDef *voidType = classContext.construct->voidType.get();
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

        // look for a matching constructor
        bool useDefaultCons = false;
        FuncDefPtr baseInit = overloads->getSigMatch(args, true);
        if (!baseInit || baseInit->getOwner() != ibase->get()) {
            
            // we must get a default initializer and it must be specific to the 
            // base class (not inherited from an ancestor of the base class)
            useDefaultCons = true;
            baseInit = overloads->getNoArgMatch(false);
            if (!baseInit || baseInit->getOwner() != ibase->get())
                classContext.error(SPUG_FSTR("Cannot create a default "
                                              "constructor because base "
                                              "class " << 
                                              (*ibase)->name <<
                                              " has no default constructor."
                                             )
                                );
        }

        FuncCallPtr funcCall =
            classContext.builder.createFuncCall(baseInit.get());
        funcCall->receiver = thisRef;
        
        // construct an argument list if we're not using the default arguments
        if (!useDefaultCons && args.size()) {
            for (int i = 0; i < args.size(); ++i)
                funcCall->args.push_back(
                    funcContext->builder.createVarRef(args[i].get())
                );
        }
        
        funcCall->emit(*funcContext);
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
    classContext.addDef(newFunc.get());
    return newFunc;
}

FuncDefPtr TypeDef::createDefaultInit(Context &classContext) {
    ArgVec args(0);
    return createOperInit(classContext, args);
}

void TypeDef::createDefaultDestructor(Context &classContext) {
    assert(classContext.ns.get() == this); // needed for final addDef()
    ContextPtr funcContext = classContext.createSubContext(Context::local);
    funcContext->toplevel = true;

    // create the "this" variable
    ArgDefPtr thisDef = classContext.builder.createArgDef(this, "this");
    funcContext->addDef(thisDef.get());
    
    FuncDef::Flags flags = 
        FuncDef::method | 
        (hasVTable ? FuncDef::virtualized : FuncDef::noFlags);
    
    // check for an override
    FuncDefPtr override = classContext.lookUpNoArgs("oper del", true, this);
    
    ArgVec args(0);
    TypeDef *voidType = classContext.construct->voidType.get();
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
    classContext.addDef(delFunc.get());
}

void TypeDef::createNewFunc(Context &classContext, FuncDef *initFunc) {
    ContextPtr funcContext = classContext.createSubContext(Context::local);
    funcContext->toplevel = true;
    funcContext->returnType = this;
    
    // copy the original arg list
    ArgVec args;
    for (ArgVec::iterator iter = initFunc->args.begin();
         iter != initFunc->args.end();
         ++iter
         ) {
        ArgDefPtr argDef =
            classContext.builder.createArgDef((*iter)->type.get(), 
                                              (*iter)->name
                                              );
        args.push_back(argDef);
        funcContext->addDef(argDef.get());
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
    for (ArgVec::iterator iter = args.begin(); iter != args.end();
         ++iter
         )
        initFuncCall->args.push_back(new VarRef(iter->get()));
    initFuncCall->receiver = thisRef;
    initFuncCall->emit(*funcContext);
    
    // return the resulting object and close the new function
    classContext.builder.emitReturn(*funcContext, thisRef.get());
    classContext.builder.emitEndFunc(*funcContext, newFunc.get());

    // register it in the class
    classContext.addDef(newFunc.get());
}

void TypeDef::createCast(Context &outer, bool throws) {
    assert(hasVTable && "Attempt to createCast() on a non-virtual class");
    ContextPtr funcCtx = outer.createSubContext(Context::local);
    funcCtx->toplevel = true;
    funcCtx->returnType = this;
    
    ArgVec args;
    args.reserve(2);
    args.push_back(
        outer.builder.createArgDef(outer.construct->vtableBaseType.get(),
                                   "val"
                                   )
    );
    
    // if this isn't the throwing variety, add a "defaultValue" arg.
    if (!throws)
        args.push_back(
            outer.builder.createArgDef(this, "defaultValue")
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
    //      __CrackBadCast(val.class, ThisClass);
    
    // val.class
    VarRefPtr valRef = funcCtx->builder.createVarRef(args[0].get());
    FuncDefPtr f = funcCtx->lookUpNoArgs("oper class", false);
    assert(f && "oper class missing");
//  XXX this was trace code that mysteriously seg-faults: since I think there 
//  might be some memory corruption happening, I'm leaving this until I can 
//  investigate.
//    string s = f->receiverType->name;
//    std::cerr << "Got oper class for " << s << endl;
    FuncCallPtr call = funcCtx->builder.createFuncCall(f.get());
    call->receiver = valRef;
    ExprPtr valClass = call;
    valClass = valClass->emit(*funcCtx);
    
    // $.isSubclass(ThisClass)
    FuncCall::ExprVec isSubclassArgs(1);
    isSubclassArgs[0] = funcCtx->builder.createVarRef(this);
    f = funcCtx->lookUp("isSubclass", isSubclassArgs, type.get());
    assert(f && "isSubclass missing");
    call = funcCtx->builder.createFuncCall(f.get());
    call->args = isSubclassArgs;
    call->receiver = valClass;

    // if ($)
    BranchpointPtr branchpoint = funcCtx->builder.emitIf(*funcCtx, call.get());
    
    // return ThisClass.unsafeCast(val);
    FuncCall::ExprVec unsafeCastArgs(1);
    unsafeCastArgs[0] = valRef;
    f = funcCtx->lookUp("unsafeCast", unsafeCastArgs, type.get());
    assert(f && "unsafeCast missing");
    call = funcCtx->builder.createFuncCall(f.get());
    call->args = unsafeCastArgs;
    funcCtx->builder.emitReturn(*funcCtx, call.get());

    // else    
    branchpoint = funcCtx->builder.emitElse(*funcCtx, branchpoint.get(), true);

    if (throws) {
        // __CrackBadCast(val.class, ThisClass);
        FuncCall::ExprVec badCastArgs(2);
        badCastArgs[0] = valClass;
        badCastArgs[1] = funcCtx->builder.createVarRef(this);
        f = outer.getParent()->lookUp("__CrackBadCast", badCastArgs);
        assert(f && "__CrackBadCast missing");
        call = funcCtx->builder.createFuncCall(f.get());
        call->args = badCastArgs;
        funcCtx->createCleanupFrame();
        call->emit(*funcCtx)->handleTransient(*funcCtx);
        funcCtx->closeCleanupFrame();
        
        // need to "return null" to provide a terminator.
        TypeDef *vp = outer.construct->voidptrType.get();
        ExprPtr nullVal = (new NullConst(vp))->convert(*funcCtx, this);
        funcCtx->builder.emitReturn(*funcCtx, nullVal.get());
    } else {
        // return defaultVal;
        VarRefPtr defaultValRef = funcCtx->builder.createVarRef(args[1].get());
        funcCtx->builder.emitReturn(*funcCtx, defaultValRef.get());
    }

    // end of story.
    funcCtx->builder.emitEndIf(*funcCtx, branchpoint.get(), true);
    funcCtx->builder.emitEndFunc(*funcCtx, castFunc.get());
    
    // add the cast function to the meta-class
    outer.addDef(castFunc.get(), type.get());
}

bool TypeDef::gotAbstractFuncs(vector<FuncDefPtr> *abstractFuncs,
                               TypeDef *ancestor
                               ) {
    bool gotAbstract = false;
    if (!ancestor)
        ancestor = this;

    // iterate over the definitions, locate all abstract functions
    for (VarDefMap::iterator iter = ancestor->beginDefs();
         iter != ancestor->endDefs(); 
         ++iter
         ) {
        OverloadDef *ovld = OverloadDefPtr::rcast(iter->second);
        if (ovld && hasAbstractFuncs(ovld, abstractFuncs)) {
            if (abstractFuncs)
                gotAbstract = true;
            else
                return true;
        }
    }
    
    // recurse through all of the parents
    TypeDefPtr parent;
    for (int i = 0; parent = ancestor->getParent(i++);)
        if (gotAbstractFuncs(abstractFuncs, parent.get()))
            if (abstractFuncs)
                gotAbstract = true;
            else
                return true;
    
    return gotAbstract;
}

void TypeDef::aliasBaseMetaTypes() {
    for (TypeVec::iterator base = parents.begin();
         base != parents.end();
         ++base
         ) {
        TypeDef *meta = (*base)->type.get();
        assert(meta != base->get());
        for (VarDefMap::iterator var = meta->beginDefs();
             var != meta->endDefs();
             ++var
             ) {
            // add all overloads that we haven't already defined.
            // XXX this check is extremely lame.  First of all, we should be 
            // separating namespace qualification from attribute/method access 
            // and we should probably do so explicitly.  Secondly, if we were 
            // going to continue in the current direction, what we need here 
            // is to do our checking at the signature level for each function, 
            // and allow Parser's addDef() to override existing values.
            if (OverloadDefPtr::rcast(var->second) && 
                !type->lookUp(var->first) &&
                var->first != "cast")
                type->addAlias(var->second.get());
        }
    }
}

void TypeDef::rectify(Context &classContext) {
    
    // if this is an abstract class, make sure we have abstract methods.  If 
    // it is not, make sure we don't have abstract methods.
    if (abstract && !gotAbstractFuncs()) {
        classContext.warn(SPUG_FSTR("Abstract class " << name << 
                                     " has no abstract functions."
                                    )
                          );
    } else if (!abstract) {
        vector<FuncDefPtr> funcs;
        if (gotAbstractFuncs(&funcs)) {
            ostringstream tmp;
            tmp << "Non-abstract class " << name << 
                " has abstract methods:\n";
            for (int i = 0; i < funcs.size(); ++i)
                tmp << "  " << *funcs[i] << '\n';
            classContext.error(tmp.str());
        }
    }
    
    // if there are no init functions specific to this class, create a
    // default constructor and possibly wrap it in a new function.
    if (!lookUp("oper init", false)) {
        FuncDefPtr initFunc = createDefaultInit(classContext);
        if (!abstract)
            createNewFunc(classContext, initFunc.get());
    }
    
    // if the class doesn't already define a delete operator specific to the 
    // class, generate one.
    FuncDefPtr operDel = classContext.lookUpNoArgs("oper del");
    if (!operDel || operDel->getOwner() != this)
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

FuncDefPtr TypeDef::getConverter(Context &context, const TypeDef &other) {
    return context.lookUpNoArgs("oper to " + other.getFullName(), true, this);
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
        // initializers.
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
        ArgVec args;
        FuncDefPtr baseInit = overloads->getSigMatch(args);
        if (!baseInit || baseInit->getOwner() != base)
            context.error(SPUG_FSTR("Cannot initialize base classes "
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
        
        SPUG_CHECK(initializer->type->isDerivedFrom(ivar->type.get()),
                   "initializer for " << ivar->name << " should be of type " <<
                    ivar->type->getDisplayName() << 
                    " but is of incompatible type  " <<
                    initializer->type->getDisplayName()
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
        FuncDefPtr operDel = context.lookUpNoArgs("oper del", true, base);
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

string TypeDef::getSpecializedName(TypeVecObj *types, bool fullName) {
    // construct the module name from the class name plus type parameters
    ostringstream tmp;
    tmp << (fullName ? getFullName() : name) << '[';
    int last = types->size()-1;
    for (int i = 0; i <= last; ++i) {
        tmp << (*types)[i]->getFullName();
        if (i != last)
            tmp << ',';
    }
    tmp << ']';
    return tmp.str();
}

void instantiateGeneric(TypeDef *type, Context &context, Context &localCtx,
                        TypeDef::TypeVecObj *types
                        ) {
    // alias all global symbols in the original module.  For the compile 
    // namespace, just reuse that of the generic.
    localCtx.ns->aliasAll(type->genericInfo->ns.get());
    localCtx.compileNS = type->genericInfo->compileNS;

    // alias the template arguments to their parameter names
    for (int i = 0; i < types->size(); ++i)
        localCtx.ns->addAlias(type->genericInfo->parms[i]->name, 
                              (*types)[i].get()
                              );
    
    istringstream fakeInput;
    Toker toker(fakeInput, "ignored-name");
    type->genericInfo->replay(toker);
    toker.putBack(Token(Token::ident, type->name, Location()));
    toker.putBack(Token(Token::classKw, "class", Location()));
    if (type->abstract)
        localCtx.nextClassFlags =
            static_cast<TypeDef::Flags>(model::TypeDef::explicitFlags |
                                        model::TypeDef::abstractClass
                                        );

    Location instantiationLoc = context.getLocation();
    if (instantiationLoc)
        localCtx.pushErrorContext(SPUG_FSTR("in generic instantiation "
                                            "at " << instantiationLoc
                                            )
                                  );
    else
        // XXX I think we should never get here now that we're 
        // materializing generic modules.
        localCtx.pushErrorContext(SPUG_FSTR("In generic instantiation "
                                            "from compiled module "
                                            "(this should never "
                                            "happen!)"
                                            )
                                  );
    Parser parser(toker, &localCtx);
    parser.parse();
    localCtx.popErrorContext();
}

namespace {
    class DummyModuleDef : public ModuleDef {
        public:
            DummyModuleDef(const string &name, Namespace *ns) :
                ModuleDef(name, ns) {
            }
            virtual void callDestructor() {}
            virtual void runMain(builder::Builder &builder) {}
            virtual bool isHiddenScope() { return true; }
    };
}

TypeDefPtr TypeDef::getSpecialization(Context &context, 
                                      TypeDef::TypeVecObj *types
                                      ) {
    assert(genericInfo);
    
    // check the type's specialization cache
    TypeDef *result = findSpecialization(types);
    if (result) {
        // record a dependency on the owner's module and return the result.
        context.recordDependency(result->getOwner()->getModule().get());
        return result;
    }
    
    // construct the module name from the class name plus type parameters
    string moduleName = getSpecializedName(types, true);
    string newTypeName = getSpecializedName(types, false);
    
    // the name that the specialization will be stored as in the 
    // specialization module.  This varies depending on whether we are 
    // building the specialization or loading from the precompiled module cache.
    string nameInModule;

    // see if the new type needs to be in a copersistent module.
    bool copersistent = false;
    ModuleDefPtr currentModule = context.ns->getModule();
    ModuleDefPtr currentMaster = currentModule->getMaster();
    SPUG_FOR(TypeVec, typeIter, *types) {
        if ((*typeIter)->getModule()->getMaster() == currentMaster)
            copersistent = true;
    }
    copersistent = copersistent || getModule()->getMaster() == currentMaster;

    // check the precompiled module cache.  We don't do this for copersistent 
    // modules: if there is an existing copy in the cache, we can't depend on 
    // it because it's from a non-copersistent version.
    ModuleDefPtr module;
    if (!copersistent)
        module = context.construct->getCachedModule(moduleName);

    if (!module) {

        // make sure we've got the right number of arguments
        if (types->size() != genericInfo->parms.size())
            context.error(SPUG_FSTR("incorrect number of arguments for "
                                    "generic " << moduleName << 
                                    ".  Expected " <<
                                    genericInfo->parms.size() << " got " <<
                                    types->size()
                                    )
                        );
        
        // if any of the parameters of the generic or the generic itself are 
        // in a hidden scope, we don't want to create an ephemeral module for 
        // it.
        bool hidden = false;
        if (isHidden()) {
            hidden = copersistent = true;
        } else {
            for (int i = 0; i < types->size(); ++i) {
                if ((*types)[i]->isHidden()) {
                    hidden = copersistent = true;
                    break;
                }
            }
        }
        if (hidden) {
            ModuleDefPtr dummyMod = new DummyModuleDef(moduleName, 
                                                       context.ns.get()
                                                       );
            // create a dummy module in the current context.
            dummyMod->setOwner(
                genericInfo->getInstanceModuleOwner(context.isGeneric()).get()
            );
            ContextPtr instantiationContext = 
                context.createSubContext(Context::module, dummyMod.get());
            instantiationContext->toplevel = true;
            instantiationContext->generic = true;
            instantiateGeneric(this, context, *instantiationContext, types);
            
            // The dummy module may have picked up some new dependencies which 
            // we need to transfer to the current module.  These are not 
            // compile-time dependencies, so we could possibly optimize them 
            // into a separate category.
            ModuleDefPtr curModule = context.ns->getModule();
            SPUG_FOR(ModuleDefMap, iter, dummyMod->dependencies)
                curModule->addDependency(iter->second.get());

            return extractInstantiation(dummyMod.get(), types);
        }
        
        // create an ephemeral module for the new class
        Context *rootContext = context.construct->rootContext.get();
        NamespacePtr compileNS =
            new GlobalNamespace(rootContext->compileNS.get(),
                                moduleName
                                );
        BuilderPtr moduleBuilder = 
            context.construct->rootBuilder->createChildBuilder();
        ContextPtr modContext =
            new Context(*moduleBuilder, Context::module, rootContext,
                        new GlobalNamespace(rootContext->ns.get(),
                                            moduleName
                                            )
                        );
        modContext->toplevel = true;
        modContext->generic = true;
        
        // create the new module with the current module as the owner.  Use 
        // the newTypeName instead of moduleName, since this is how we should 
        // have done it for modules in the first place and we're going to 
        // override the canonical name later anyway.
        if (!currentModule)
            cout << "no current module for " << newTypeName << endl;
        module = modContext->createModule(newTypeName, "", 
                                          copersistent ? currentModule.get() : 
                                                         0
                                          );
        
        // Add the modules of the parameter types as dependents.
        SPUG_FOR(TypeVecObj, typeIter, *types)
            module->addDependency((*typeIter)->getModule().get());
        
        // XXX this is confusing: there's a "owner" that's part of some kinds of 
        // ModuleDef that's different from VarDef::owner - we set VarDef::owner 
        // here so that we can accept protected variables from the original 
        // module's context
        ModuleDefPtr owner = genericInfo->ns->getRealModule();
        module->setOwner(
            genericInfo->getInstanceModuleOwner(context.isGeneric()).get()
        );

        // Fix up the canonical name of the module.  The previous setOwner() 
        // sets the canonical as if the module were directly owned by the 
        // parent module, but that may not be the case if the generic is 
        // defined in a nested context (e.g. in a class).
        module->setNamespaceName(moduleName);
        
        instantiateGeneric(this, context, *modContext, types);
        
        // after we're done parsing, change the owner to the actual owner so 
        // that names are generated correctly.
        
        // use the source path of the owner
        module->sourcePath = owner->sourcePath;
        module->sourceDigest = owner->sourceDigest;
        result = extractInstantiation(module.get(), types);

        module->cacheable = true;    
        module->close(*modContext);

        // store the module in the in-memory cache
        context.construct->registerModule(module.get());

        nameInModule = name;
    } else {
        nameInModule = newTypeName;
        result = extractInstantiation(module.get(), types);
    }

    // record a dependency on the owner's module
    if (!copersistent)
        context.ns->getModule()->addDependency(module.get());
    
    return result;
}

bool TypeDef::isConstant() {
    return true;
}

void TypeDef::getDependents(std::vector<TypeDefPtr> &deps) {}

void TypeDef::dump(ostream &out, const string &prefix) const {
    out << prefix << "class " << getFullName() << " {" << endl;
    string childPrefix = prefix + "  ";
    
    for (TypeVec::const_iterator baseIter = parents.begin();
         baseIter != parents.end();
         ++baseIter
         ) {
        out << childPrefix << "parent:" << endl;
        (*baseIter)->dump(out, childPrefix+"  ");
    }
    
    for (VarDefMap::const_iterator iter = beginDefs(); iter != endDefs();
         ++iter
         )
        iter->second->dump(out, childPrefix);
    out << prefix << "}" << endl;
}

bool TypeDef::needsReceiver() const {
    // Types never need a receiver.
    return false;
}

bool TypeDef::isSerializable() const {
    if (meta)
        return false;
    else
        return VarDef::isSerializable();
}

void TypeDef::addDependenciesTo(ModuleDef *mod, VarDef::Set &added) const {
    // if we've already dealt with this type, quit.
    if (!added.insert(this).second)
        return;

    mod->addDependency(VarDef::getModule());

    // compute dependencies from all non-private symbols
    for (VarDefMap::const_iterator iter = defs.begin(); iter != defs.end(); 
         ++iter
         ) {
        if (iter->first.compare(0, 2, "__"))
            iter->second->addDependenciesTo(mod, added);
    }
}

void TypeDef::serializeExtern(Serializer &serializer) const {
    VarDef::serializeExternRef(serializer, &genericParms);
}

void TypeDef::serializeDef(Serializer &serializer) const {
    if (Serializer::trace)
        cerr << ">> Serializing the body of class " << getFullName() << endl;
    int objectId = serializer.getObjectId(this);
    SPUG_CHECK(objectId != -1,
               "Type " << getFullName() << " was not registered in the "
                "declarations for this module."
               );
    serializer.write(objectId, "objectId/2");
    
    if (generic) {
        Serializer::StackFrame<Serializer> digestState(serializer, false);
        genericInfo->serialize(serializer);
    } else {
        serializer.write(parents.size(), "#bases");
            
        for (TypeVec::const_iterator i = parents.begin();
             i != parents.end();
             ++i
             )
            (*i)->serialize(serializer, false, 0);

        // serialize the optional fields for generic instantiations.
        if (templateType) {
            ostringstream temp;
            Serializer sub(serializer, temp);
            sub.write(CRACK_PB_KEY(1, ref), "templateType.header");
            templateType->serialize(sub, false, 0);
            for (TypeVec::const_iterator iter = genericParms.begin();
                 iter != genericParms.end();
                 ++iter
                 ) {
                // field id = 2 (<< 3) | type = 3 (reference)
                sub.write(CRACK_PB_KEY(2, ref), 
                          "genericParms[i].header"
                          );
                (*iter)->serialize(sub, false, 0);
            }
            serializer.write(temp.str(), "optional");
        } else {
            serializer.write(0, "optional");
        }
        
        Namespace::serializeNonTypeDefs(serializer);
    }
    if (Serializer::trace)
        cerr << ">> Done serializing " << getFullName() << endl;
}

void TypeDef::serializeAlias(Serializer &serializer, 
                             const string &alias
                             ) const {
    serializer.write(Serializer::typeAliasId, "kind");
    serializer.write(alias, "alias");
    serializeExternRef(serializer, 0);
}

void TypeDef::serialize(Serializer &serializer, bool writeKind,
                        const Namespace *ns
                        ) const {
    SPUG_CHECK(!writeKind, "need to write kind for type " << getFullName());
    if (serializer.writeObject(this, "type")) {
        ModuleDefPtr module = VarDef::getModule();
        if (module != serializer.module) {

            // write an "Extern" (but not a reference, we're already in a 
            // reference to the object we'd be externing)
            if (templateType) {
                
                // If this is a generic instantiation, write the base type 
                // with our parameters.
                templateType->serializeExternCommon(serializer, 
                                                    &genericParms
                                                    );
            } else {
                serializeExternCommon(serializer, 0);
            }
        } else {
            // For local types, this function should always just produce an 
            // object reference because local types are declared before defs.  
            // If we got here, something's wrong.
            SPUG_CHECK(false,
                       "Serializing full type " << getFullName() << 
                        " from a reference."
                       );
        }
    }
}

void TypeDef::serializeDecl(Serializer &serializer, ModuleDef *master) {
    if (serializer.writeObject(this, "decl")) {
        serializer.write(name, "name");

        // Flags.
        int flags = (pointer ? 1 : 0) |
                    (hasVTable ? 2 : 0) |
                    (abstract ? 4 : 0) |
                    (generic ? 8 : 0);
        serializer.write(flags, "flags");

        {
            ostringstream temp;
            Serializer sub(serializer, temp);
            
            // If we're module-scoped to a slave module, record the owner.
            ModuleDefPtr module = ModuleDefPtr::cast(getOwner());
            if (module && module->isSlave()) {
                sub.write(CRACK_PB_KEY(2, ref), "owner.header");
                module->serializeSlaveRef(sub);
            }
            serializer.write(temp.str(), "optional");
        }
        int result = serializer.registerObject(this);
        serializeTypeDecls(serializer, master);
    }
}

namespace {
    struct TypeDefReader : public Deserializer::ObjectReader {
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            return VarDef::deserializeTypeAliasBody(deser);
        }
    };
} // anon namespace

TypeDefPtr TypeDef::deserializeRef(Deserializer &deser, const char *name) {
    Deserializer::ReadObjectResult readObj =
        deser.readObject(TypeDefReader(), name ? name : "type");
    return TypeDefPtr::arcast(readObj.object);
}

namespace {
    
    void materializeOneCastFunc(Context &metaClassContext, TypeDef *type,
                                const ArgVec &args
                                ) {
        FuncDefPtr func = metaClassContext.builder.materializeFunc(
            metaClassContext, 
            FuncDef::noFlags, 
            "cast", 
            type, 
            args
        );
        metaClassContext.addDef(func.get());
    }

    void materializeCastFuncs(Context &classContext, TypeDef *type) {
        ContextPtr metaClassContext =
            classContext.createSubContext(Context::instance, type->type.get());
                                           
        ArgVec args;
        args.reserve(2);
        args.push_back(
            classContext.builder.createArgDef(
                classContext.construct->vtableBaseType.get(),
                "val"
            )
        );

        materializeOneCastFunc(*metaClassContext, type, args);
    
        // Now materialize the two argument form.
        args.push_back(
            classContext.builder.createArgDef(type, "defaultValue")
        );
        materializeOneCastFunc(*metaClassContext, type, args);
   }
}

TypeDefPtr TypeDef::deserializeTypeDef(Deserializer &deser, const char *name) {
    // Read the object id and retrieve the existing object.
    int objectId = deser.readUInt("objectId/2");
    TypeDefPtr type = deser.getObject(objectId);
    SPUG_CHECK(type, "Type object " << objectId << " not registered.");
    if (Serializer::trace)
        cerr << ">> deserializing body of type " << type->getFullName() << 
            endl;

    // Read a generic or a real class.
    if (type->generic) {
        Serializer::StackFrame<Deserializer> digestState(deser, false);
        type->genericInfo = Generic::deserialize(deser);
        type->genericInfo->ns = deser.context->ns.get();
        type->genericInfo->seedCompileNS(*deser.context);
    } else {
        // bases
        int count = deser.readUInt("#bases");
        TypeDef::TypeVec bases(count);
        for (int i = 0; i < count; ++i)
            bases[i] = TypeDef::deserializeRef(deser, "bases[i]");
    
        type->parents = bases;
        
        // check for optional fields
        CRACK_PB_BEGIN(deser, 256, optional);
            CRACK_PB_FIELD(1, ref)
                type->templateType = 
                    deserializeRef(optionalDeser, "templateType").get();
                break;
            CRACK_PB_FIELD(2, ref)
                type->genericParms.push_back(
                    deserializeRef(optionalDeser, "genericParms")
                );
                break;
        CRACK_PB_END

        // If it's a generic instantiation, add it to it's template's 
        // specialization cache.
        if (type->templateType) {
            TypeVecObjPtr types = new TypeVecObj(type->genericParms);
            (*type->templateType->generic)[types.get()] = type;
        }
    
        ContextPtr classContext =
            deser.context->createSubContext(Context::instance,
                                            type.get(),
                                            &type->name
                                            );
        // add the "cast" methods (This is duplicated in the parser, refactor)
        if (type->hasVTable)
            materializeCastFuncs(*classContext, type.get());

        // 'defs' - fill in the body.
        ContextStackFrame<Deserializer> cstack(deser, classContext.get());
        type->deserializeDefs(deser);

        // If we need to reconstruct the vtable, give the type the chance to 
        // do that here.        
        if (type->hasVTable)
            type->materializeVTable(*deser.context);
    }
    if (Serializer::trace)
        cerr << ">> done deserializing type " << type->getFullName() << endl;
    
    type->complete = true;
    return type;
}

namespace {
    struct TypeDeclReader : Deserializer::ObjectReader {
        spug::RCBasePtr read(Deserializer &deser) const {
            string name = deser.readString(Serializer::varNameSize, "name");
            NamespacePtr owner = deser.context->ns;

            // Read the flags.
            int flags = deser.readUInt("flags");
            bool isGeneric = (flags & 8) ? true : false; 
        
            // Deserialize optional fields.
            CRACK_PB_BEGIN(deser, 256, optional)
                CRACK_PB_FIELD(2, ref) {
                    ModuleDefPtr mod = deser.context->ns;
                    owner = mod->deserializeSlaveRef( optionalDeser);
                    break;
                }
            CRACK_PB_END
        
            TypeDefPtr type;
            if (isGeneric) {
                type = new TypeDef(deser.context->construct->classType.get(), 
                                   name, 
                                   true
                                   );
                
                // We mainly initialize this here as an indicator that we 
                // expect a generic for when we deserialize the full 
                // definition later.
                type->generic = new TypeDef::SpecializationCache();
            } else {
                // Create a subcontext linked the class' owner so 
                // type materialization works for slave modules.  At this time 
                // we need to do this so that the meta-class is properly 
                // registered.
                ContextPtr classContext =
                    deser.context->createSubContext(Context::instance, 
                                                    owner.get()
                                                    );
                type = classContext->builder.materializeType(
                    *classContext,
                    name,
                    owner->getNamespaceName()
                );
            }

            // Add the flags.
            type->pointer = (flags & 1) ? true : false;
            type->hasVTable = (flags & 2) ? true : false;
            type->abstract = (flags & 4) ? true : false;

            owner->addDef(type.get());
            return type;
        }
    };
}

void TypeDef::deserializeDecl(Deserializer &deser) {
    Deserializer::ReadObjectResult result =
        deser.readObject(TypeDeclReader(), "decl");
    if (result.definition) {
        TypeDefPtr type = result.object;
        NamespacePtr owner = type->getOwner();
            
        // do the nested declarations against the new context.
        ContextPtr classContext = 
            deser.context->createSubContext(Context::instance, type.get(),
                                            &type->name
                                            );
        ContextStackFrame<Deserializer> frame(deser, classContext.get());
        deserializeTypeDecls(deser);
    }
}
