// Copyright 2010 Google Inc.

#include "Type.h"

#include <assert.h>
#include <iostream>
#include "builder/Builder.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "Func.h"

using namespace crack::ext;
using namespace model;
using namespace std;

Type::Impl::~Impl() {
    // we don't have to clean up "bases" - since these are types, they should
    // get cleaned up by the module.
    
    // clean up the funcs
    for (FuncMap::iterator i = funcs.begin(); i != funcs.end(); ++i)
        delete i->second;
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

Func *Type::addMethod(Type *returnType, const std::string &name,
                      void *funcPtr
                      ) {
    Func *result = new Func(0, returnType, name, funcPtr, Func::method);
    impl->funcs[name] = result;
    return result;
}

Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                            void *funcPtr
                            ) {
    Func *result = new Func(0, returnType, name, funcPtr, Func::noFlags);
    impl->funcs[name] = result;
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

    // emit all of the method defs
    for (FuncMap::iterator fi = impl->funcs.begin(); fi != impl->funcs.end();
         ++fi
         ) {
        // bind the class context to the function, then finish it.
        fi->second->context = clsCtx.get();
        fi->second->finish();
    }
    
    ctx->builder.emitEndClass(*clsCtx);
    ctx->ns->addDef(typeDef);
}
