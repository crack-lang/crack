// Copyright 2010-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Arno Rehn <arno@arnorehn.de>
// Copyright 2011-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Module.h"

#include <string.h>
#include <sstream>
#include "builder/Builder.h"
#include "model/ConstVarDef.h"
#include "model/Context.h"
#include "model/FloatConst.h"
#include "model/IntConst.h"
#include "model/Namespace.h"
#include "model/OverloadDef.h"
#include "parser/Toker.h"
#include "parser/Parser.h"
#include "Type.h"
#include "Func.h"

using namespace std;
using namespace crack::ext;
using namespace parser;
using namespace model;

Module::Module(Context *context) : context(context), finished(false) {
    memset(builtinTypes, 0, sizeof(builtinTypes));
}

Module::~Module() {
    int i;

    // cleanup the builtin types
    for (i = 0; i < sizeof(builtinTypes) / sizeof(Type *); ++i)
        delete builtinTypes[i];
    
    // cleanup new or looked up types
    for (TypeMap::iterator iter = types.begin(); iter != types.end(); ++iter)
         delete iter->second;
    
    // cleanup the funcs
    for (i = 0; i < funcs.size(); ++i)
        delete funcs[i];
}

#define GET_TYPE(capName, lowerName)                                        \
    Type *Module::get##capName##Type() {                                    \
        return builtinTypes[lowerName##Type] ?                              \
            builtinTypes[lowerName##Type] :                                 \
            (builtinTypes[lowerName##Type] =                                \
              new Type(this, context->construct->lowerName##Type.get()));   \
    }


GET_TYPE(Class, class)
GET_TYPE(Void, void)
GET_TYPE(Voidptr, voidptr)
GET_TYPE(Bool, bool)
GET_TYPE(Byteptr, byteptr)
GET_TYPE(Byte, byte)
GET_TYPE(Int16, int16)
GET_TYPE(Int32, int32)
GET_TYPE(Int64, int64)
GET_TYPE(Int, int)
GET_TYPE(Intz, intz)
GET_TYPE(Uint16, uint16)
GET_TYPE(Uint32, uint32)
GET_TYPE(Uint64, uint64)
GET_TYPE(Uint, uint)
GET_TYPE(Uintz, uintz)
GET_TYPE(Float32, float32)
GET_TYPE(Float64, float64)
GET_TYPE(Float, float)
GET_TYPE(VTableBase, vtableBase)
GET_TYPE(Object, object)
GET_TYPE(String, string)
GET_TYPE(StaticString, staticString)

namespace {

// internal type for types with vtables.
class VTableType : public Type {
    
    private:
        string proxyName;
    
    public:
        VTableType(Module *module, const string &name, Context *context,
                   size_t size,
                   TypeDef *typeDef = 0
                   ) :
            Type(module, name + ":ExtUser", context, size, typeDef),
            proxyName(name) {
        }

        // propagate all constructors from 'inner' to 'outer'
        void propagateConstructors(Context &context, TypeDef *inner, 
                                   TypeDef *outer
                                   ) {
            
            OverloadDefPtr constructors = 
                OverloadDefPtr::rcast(inner->lookUp("oper init"));
            
            if (!constructors)
                return;
            
            for (OverloadDef::FuncList::iterator fi =
                  constructors->beginTopFuncs();
                 fi != constructors->endTopFuncs();
                 ++fi
                 ) {
                // create the init function, wrap it in an "oper new"
                FuncDefPtr operInit = 
                    outer->createOperInit(context, (*fi)->args);
                outer->createNewFunc(context, operInit.get());
            }
        }

        virtual void finish() {
            if (isFinished())
                return;

            // finish the inner class
            Type::finish();
            
            TypeDef::TypeVec bases;
            bases.reserve(2);
            bases.push_back(module->getVTableBaseType()->typeDef);
            bases.push_back(typeDef);

            // create the subcontext and emit the beginning of the class.
            Context *ctx = impl->context;
            ContextPtr clsCtx = 
                new Context(ctx->builder, Context::instance, ctx, 0,
                            ctx->compileNS.get()
                );
            TypeDefPtr td =
                ctx->builder.emitBeginClass(*clsCtx, proxyName, bases, 0);
            ctx->ns->addDef(td.get());
            td->aliasBaseMetaTypes();

            // wrap all of the constructors
            propagateConstructors(*clsCtx, typeDef, td.get());
            
            // wrap all of the vwrapped methods
            for (FuncVec::iterator fi = impl->funcs.begin(); 
                 fi != impl->funcs.end();
                 ++fi
                 ) {
                if ((*fi)->getVWrap()) {
                    setClasses(*fi, typeDef, td.get(), clsCtx.get());
                    (*fi)->finish();
                }
            }

            ctx->builder.emitEndClass(*clsCtx);
            
            // replace the inner type with the outer type (we can be cavalier 
            // about discarding the reference here because Type::finish() 
            // should have assigned this to a variable)
            typeDef = td.get();
        }        
};

} // anon namespace

Type *Module::getType(const char *name) {
    
    // try looking it up in the map, first
    TypeMap::iterator iter = types.find(name);
    if (iter != types.end())
        return iter->second;
    
    TypeDefPtr rawType = TypeDefPtr::rcast(context->ns->lookUp(name));
    if (!rawType) {
        return 0;
    }
    
    // Create a new type, add it to the map.
    Type *type = new Type(this, rawType.get());
    types[name] = type;

    return type;
}

Type *Module::addTypeWorker(const char *name, size_t instSize, bool forward,
                            bool hasVTable
                            ) {

    TypeMap::iterator i = types.find(name);
    bool found = (i != types.end());
    if (found && !(i->second->typeDef && i->second->typeDef->forward))  {
        std::cerr << "Type " << name << " already registered!" << std::endl;
        assert(false);
    }

    Type *result;

    if (forward) {
        // we can't currently combine forward and hasVTable because our inner 
        // TypeDef changes for a vtable class.
        assert(!hasVTable);
        result = new Type(this, name, context, instSize, 
                          context->createForwardClass(name).get()
                          );
    } else if (hasVTable) {
        result = new VTableType(this, name, context, instSize, 0);
    } else {
        result = new Type(this, name, context, instSize);
    }

    types[name] = result;
    return result;
}

Type *Module::addType(const char *name, size_t instSize, bool hasVTable) {
    return Module::addTypeWorker(name, instSize, false, hasVTable);
}

Type *Module::addForwardType(const char *name, size_t instSize) {
    return Module::addTypeWorker(name, instSize, true, false);
}

Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr,
                      const char *symbolName) {
    assert(!finished && "Attempting to add a function to a finished module.");
    returnType->checkFinished();
    Func *f = new Func(context, returnType, name, funcPtr, Func::noFlags);
    if (symbolName)
        f->setSymbolName(symbolName);
    funcs.push_back(f);
    return f;
}

Func* Module::addFunc(Type* returnType, const char* name, const std::string& body)
{
    assert(!finished && "Attempting to add a function to a finished module.");
    returnType->checkFinished();
    Func *f = new Func(context, returnType, name, body, Func::noFlags);
    funcs.push_back(f);
    return f;
}

void Module::addConstant(Type *type, const std::string &name, double val) {
    type->checkFinished();
    FloatConstPtr valObj =
        context->builder.createFloatConst(*context, val, type->typeDef);
    vars.push_back(new ConstVarDef(type->typeDef, name, valObj));
}

void Module::addConstant(Type *type, const std::string &name, int64_t val) {
    type->checkFinished();
    IntConstPtr valObj =
        context->builder.createIntConst(*context, val, type->typeDef);
    vars.push_back(new ConstVarDef(type->typeDef, name, valObj));
}

void Module::inject(const std::string& code)
{
    std::istringstream codeStream(code);
    Toker toker(codeStream, "injected code");
    Parser parser(toker, context);
    parser.parse();
}

void Module::finish() {
    // no-op if this is already finished
    if (finished)
        return;

    for (int i = 0; i < funcs.size(); ++i)
        funcs[i]->finish();
    
    // add the variable definitions to the context.
    for (int i = 0; i < vars.size(); ++i)
        context->ns->addDef(vars[i]);

    finished = true;
}
