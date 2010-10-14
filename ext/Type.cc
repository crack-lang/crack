// Copyright 2010 Google Inc.

#include "Type.h"

#include <assert.h>
#include <iostream>
#include "builder/Builder.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "Func.h"
#include "Module.h"

using namespace crack::ext;
using namespace model;
using namespace std;

Type::Impl::~Impl() {
    // we don't have to clean up "bases" - since these are types, they should
    // get cleaned up by the module.
    
    // clean up the funcs
    for (FuncVec::iterator i = funcs.begin(); i != funcs.end(); ++i)
        delete *i;
    
    // clean up the inst vars
    for (VarMap::iterator vi = instVars.begin(); vi != instVars.end(); ++vi)
        delete vi->second;
}

Type::~Type() {
    if (impl)
        delete impl;
}

void Type::checkFinished() {
    if (!typeDef) {
        std::cerr << "Attempting to make use of unfinished type " << impl->name
            << "."  << std::endl;
        assert(false);
    }
}

void Type::addBase(Type *base) {
    base->checkFinished();
    impl->bases.push_back(base);
}

void Type::addInstVar(Type *type, const std::string &name) {
    if (impl->instVars.find(name) != impl->instVars.end()) {
        std::cerr << "variable " << name << " already exists." << endl;
        assert(false);
    }

    impl->instVars[name] = new Var(type, name);
}

Func *Type::addMethod(Type *returnType, const std::string &name,
                      void *funcPtr
                      ) {
    Func *result = new Func(0, returnType, name, funcPtr, Func::method);
    impl->funcs.push_back(result);
    return result;
}

Func *Type::addConstructor(const char *name, void *funcPtr) {
    Func *result = new Func(0, impl->module->getVoidType(), name ? name : "", 
                            funcPtr,
                            Func::constructor | Func::method
                            );
    impl->funcs.push_back(result);
    return result;
}

Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                            void *funcPtr
                            ) {
    Func *result = new Func(0, returnType, name, funcPtr, Func::noFlags);
    impl->funcs.push_back(result);
    return result;
}

void Type::finish() {
    // ignore this if we're already finished.
    if (typeDef)
        return;
    
    Context *ctx = impl->context;

    // construct the list of bases
    vector<TypeDefPtr> bases;
    vector<TypeDefPtr> ancestors;  // keeps track of all ancestors
    for (TypeVec::iterator i = impl->bases.begin(); i != impl->bases.end();
         ++i
         ) {
        // make sure we apply the same constraints as for a parsed class.
        (*i)->checkFinished();
        (*i)->typeDef->addToAncestors(*ctx, ancestors);
        bases.push_back((*i)->typeDef);
    }
    
    // create the subcontext and emit the beginning of the class.
    ContextPtr clsCtx = new Context(ctx->builder, Context::instance, ctx, 0);
    TypeDefPtr td =
        ctx->builder.emitBeginClass(*clsCtx, impl->name, bases, 0);
    typeDef = td.get();

    // emit the variables
    for (VarMap::iterator vi = impl->instVars.begin();
         vi != impl->instVars.end();
         ++vi
         ) {
        Type *type = vi->second->type;
        type->checkFinished();
        VarDefPtr varDef =
            clsCtx->builder.emitVarDef(*clsCtx, type->typeDef, 
                                       vi->second->name, 
                                       0,
                                       false
                                       );
        clsCtx->ns->addDef(varDef.get());
    }

    // emit all of the method defs
    for (FuncVec::iterator fi = impl->funcs.begin(); fi != impl->funcs.end();
         ++fi
         ) {
        // bind the class context to the function, then finish it.
        (*fi)->context = clsCtx.get();
        (*fi)->finish();
    }
    
    ctx->builder.emitEndClass(*clsCtx);
    ctx->ns->addDef(typeDef);
}
