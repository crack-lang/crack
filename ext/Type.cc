// Copyright 2010-2012 Google Inc.
// Copyright 2011 Conrad Steenberg <conrad.steenberg@gmail.com>
// Copyright 2011-2012 Arno Rehn <arno@arnorehn.de>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Type.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include "builder/Builder.h"
#include "model/CompositeNamespace.h"
#include "model/Context.h"
#include "model/OverloadDef.h"
#include "model/TypeDef.h"
#include "parser/Token.h"
#include "parser/Parser.h"
#include "Func.h"
#include "Module.h"

using namespace crack::ext;
using namespace model;
using namespace parser;
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


void Type::checkInitialized() const {
    if (!impl) {
        std::cerr << "Attempting to access attributes of forward type"
            << "."  << std::endl;
        assert(false);
    }
}

void Type::checkFinished() {
    if (!typeDef) {
        std::cerr << "Attempting to make use of unfinished type";
        if (impl) std::cerr << " " << impl->name;
        std::cerr << std::endl;
        assert(false);
    }
}

void Type::addBase(Type *base) {
    base->checkFinished();
    impl->bases.push_back(base);
}

void Type::addInstVar(Type *type, const std::string &name, size_t offset) {
    checkInitialized();
    if (impl->instVars.find(name) != impl->instVars.end()) {
        std::cerr << "variable " << name << " already exists." << endl;
        assert(false);
    }

    Var *var = new Var(type, name, offset);
    impl->instVars[name] = var;
    impl->instVarVec.push_back(var);
}

Func *Type::addMethod(Type *returnType, const std::string &name,
                      void *funcPtr
                      ) {
    checkInitialized();
    Func *result = new Func(0, returnType, name, funcPtr, Func::method);
    impl->funcs.push_back(result);
    return result;
}

Func* Type::addMethod(Type* returnType, const std::string& name,
                      const std::string& body
                      )
{
    checkInitialized();
    Func *result = new Func(0, returnType, name, body, Func::method);
    impl->funcs.push_back(result);
    return result;
}

Func *Type::addConstructor(const char *name, void *funcPtr) {
    checkInitialized();
    Func *result = new Func(0, module->getVoidType(), name ? name : "", 
                            funcPtr,
                            Func::constructor | Func::method
                            );

    impl->funcs.push_back(result);
    return result;
}

Func* Type::addConstructor(const std::string& body) {
    checkInitialized();
    Func *result = new Func(0, module->getVoidType(), "",
                            body,
                            Func::constructor | Func::method
                            );
    impl->funcs.push_back(result);
    return result;
}

Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                            void *funcPtr
                            ) {
    checkInitialized();
    Func *result = new Func(0, returnType, name, funcPtr, Func::noFlags);
    impl->funcs.push_back(result);
    return result;
}

Func* Type::addStaticMethod(Type* returnType, const std::string& name,
                            const std::string& body
                           ) {
    checkInitialized();
    Func *result = new Func(0, returnType, name, body, Func::noFlags);
    impl->funcs.push_back(result);
    return result;
}

const Type::FuncVec& Type::getMethods() const
{
    checkInitialized();
    return impl->funcs;
}

bool Type::methodHidesOverload(const string& name,
                               const vector<Type *>& args
                              ) const {
    checkInitialized();

    ArgVec realArgs; realArgs.reserve(args.size());
    for (TypeVec::const_iterator iter = args.begin(); iter != args.end();
         ++iter) {
        realArgs.push_back(new ArgDef((*iter)->typeDef, string()));
    }

    for (TypeVec::iterator iter = impl->bases.begin();
         iter != impl->bases.end(); ++iter) {
        TypeDef *td = (*iter)->typeDef;
        VarDefPtr varDef = td->lookUp(name);
        if (varDef) {
            OverloadDef *overloadDef = OverloadDefPtr::rcast(varDef);
            if (overloadDef) {
                FuncDef *funcDef = overloadDef->getSigMatch(realArgs);
                return funcDef && !funcDef->isOverridable();
            }
        }
    }

    return false;
}

Type *Type::getSpecialization(const vector<Type *> &params) {
    checkFinished();
    TypeDef::TypeVecObjPtr innerParams = new TypeDef::TypeVecObj;
    for (int i = 0; i < params.size(); ++i) {
        params[i]->checkFinished();
        innerParams->push_back(params[i]->typeDef);
    }

    TypeDefPtr spec = typeDef->getSpecialization(*module->context, 
                                                 innerParams.get());
    
    // see if it's cached in the module
    Module::TypeMap::iterator iter = module->types.find(spec->getFullName());
    if (iter != module->types.end())
        return iter->second;
    
    Type *type = new Type(module, spec.get());
    module->types[spec->getFullName()] = type;
    return type;
}

vector<Type *> Type::getGenericParams() const {
    vector<Type*> params;
    if (!typeDef)
        return params;

    params.reserve(typeDef->genericParms.size());

    for (TypeDef::TypeVec::iterator iter = typeDef->genericParms.begin();
         iter != typeDef->genericParms.end(); ++iter)
    {
        Type *associatedType = 0;

        // see if it's cached in the module
        Module::TypeMap::iterator typeMapIter =
            module->types.find((*iter)->getFullName());

        if (typeMapIter != module->types.end()) {
            associatedType = typeMapIter->second;
        } else {
            associatedType = new Type(module, iter->get());
            module->types[(*iter)->getFullName()] = associatedType;
        }

        assert(associatedType);

        params.push_back(associatedType);
    }

    return params;
}

bool Type::isPrimitive() const {
    return typeDef && !typeDef->pointer;
}

string Type::stringifyTypedef(TypeDef* td) {
    string str = td->name;

    if (!td->name.compare(0, 6, "array[")) {
        str = "array";
    }

    if (td->genericParms.size() > 0) {
        str.push_back('[');

        for (TypeDef::TypeVec::iterator iter = td->genericParms.begin();
            iter != td->genericParms.end(); ++iter)
        {
            if (iter != td->genericParms.begin()) {
                str.push_back(',');
            }
            str.append(stringifyTypedef(iter->get()));
        }

        str.push_back(']');
    }

    return str;
}

string Type::toString() const {
    return typeDef ? stringifyTypedef(typeDef) : impl->name;
}

bool Type::isFinished() const {
    return finished || (!impl && typeDef && !typeDef->forward);
}


void Type::setClasses(Func *f, model::TypeDef *base, model::TypeDef *wrapper,
                      model::Context *context
                      ) {
    f->receiverType = base;
    f->wrapperClass = wrapper;
    f->context = context;
}

void Type::injectBegin(const string& code)
{
    checkInitialized();
    impl->beginCode.append(code);
}

void Type::injectEnd(const string& code)
{
    checkInitialized();
    impl->endCode.append(code);
}

void Type::finish() {
    // ignore this if we're already finished.
    if (isFinished())
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
    ContextPtr clsCtx = new Context(ctx->builder, Context::instance, ctx, 0,
                                    ctx->compileNS.get()
                                   );
    TypeDefPtr td =
        ctx->builder.emitBeginClass(*clsCtx, impl->name, bases, typeDef);
    typeDef = td.get();
    typeDef->aliasBaseMetaTypes();

    if (!typeDef->getOwner())
        ctx->ns->addDef(typeDef);

    // create a lexical context which delegates to both the class context and
    // the parent context.
    NamespacePtr lexicalNS =
        new CompositeNamespace(typeDef, ctx->ns.get());
    ContextPtr lexicalContext =
        clsCtx->createSubContext(Context::composite, lexicalNS.get());

    // emit the variables
    for (int vi = 0; vi < impl->instVarVec.size(); ++vi) {
        Var *var = impl->instVarVec[vi];
        Type *type = var->type;
        type->checkFinished();
        VarDefPtr varDef =
            clsCtx->builder.createOffsetField(*clsCtx, type->typeDef,
                                              var->name,
                                              var->offset
                                              );
        clsCtx->ns->addDef(varDef.get());
    }

    // inject code at the beginning
    if (!impl->beginCode.empty()) {
        impl->beginCode.push_back('}');
        std::istringstream codeStream(impl->beginCode);
        Toker toker(codeStream, "injected code");
        Parser parser(toker, lexicalContext.get());
        parser.parseClassBody();
    }

    // emit all of the method defs
    for (FuncVec::iterator fi = impl->funcs.begin(); fi != impl->funcs.end();
         ++fi
         ) {
        // bind the class to the function, if it's not VWrapped, bind the 
        // context to it and finish it.  VWrapped functions get finished in 
        // the outer class.
        (*fi)->receiverType = typeDef;
        if (!(*fi)->getVWrap()) {
            (*fi)->context = lexicalContext.get();
            (*fi)->finish();
        }
    }

    // inject code at the end
    if (!impl->endCode.empty()) {
        impl->endCode.push_back('}');
        std::istringstream codeStream(impl->endCode);
        Toker toker(codeStream, "injected code");
        Parser parser(toker, lexicalContext.get());
        parser.parseClassBody();
    }

    // pad the class to the instance size
    typeDef->padding = impl->instSize;
    ctx->builder.emitEndClass(*clsCtx);

    finished = true;
}
