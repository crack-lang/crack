// Copyright 2010-2012 Google Inc.
// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Arno Rehn <arno@arnorehn.de>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Func.h"

#include "model/ArgDef.h"
#include "model/CleanupFrame.h"
#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/Initializers.h"
#include "model/ResultExpr.h"
#include "model/VarRef.h"
#include "builder/Builder.h"
#include "parser/Toker.h"
#include "parser/Parser.h"
#include "Module.h"
#include "Type.h"

#include <sstream>

using namespace crack::ext;
using namespace std;
using namespace model;
using namespace builder;
using namespace parser;

namespace crack { namespace ext {
    struct Arg {
        Type *type;
        string name;
    
        Arg(Type *type, const string &name) :
            type(type),
            name(name) {
        }
    };
}}

void Func::addArg(Type *type, const string &name) {
    assert(!finished && 
            "Attempted to add an argument to a finished function."
           );
    args.push_back(new Arg(type, name));
}

void Func::setIsVariadic(bool isVariadic)
{
    if (isVariadic) {
        flags = static_cast<Func::Flags>(flags | Func::variadic);
    } else {
        flags = static_cast<Func::Flags>(flags & ~Func::variadic);
    }
}

bool Func::isVariadic() const
{
    return flags & Func::variadic;
}

void Func::setVWrap(bool vwrapEnabled) {
    if (vwrapEnabled)
        flags = static_cast<Func::Flags>(flags | vwrap);
    else
        flags = static_cast<Func::Flags>(flags & ~vwrap);
}

bool Func::getVWrap() const {
    return flags & vwrap;
}

void Func::setVirtual(bool virtualizedEnabled) {
    if (virtualizedEnabled)
        flags = static_cast<Func::Flags>(flags | virtualized);
    else
        flags = static_cast<Func::Flags>(flags & ~virtualized);
}

// gets the "virtualized" flag
bool Func::getVirtual() const {
    return flags & virtualized;
}

void Func::setBody(const std::string& body)
{
    funcBody = body;
}

std::string Func::getBody() const
{
    return funcBody;
}

void Func::setInitializers(const string& initializers)
{
    ctorInitializers = initializers;
}

string Func::getInitializers() const
{
    return ctorInitializers;
}

unsigned int Func::getVTableOffset() const
{
    return vtableSlot;
}

void Func::finish() {
    if (finished || !context)
        return;

    // if this is a composite context, get the parent context for cases where
    // we need it directly.
    ContextPtr realCtx = context->getDefContext();

    // we're going to need this
    TypeDef *voidType = context->construct->voidType.get();

    Builder &builder = context->builder;
    std::vector<ArgDefPtr> realArgs(args.size());
    for (int i = 0; i < args.size(); ++i) {
        args[i]->type->checkFinished();
        realArgs[i] = builder.createArgDef(args[i]->type->typeDef, 
                                           args[i]->name
                                           );
    }

    FuncDefPtr funcDef;

    FuncDefPtr override;

    // XXX the functionality in checkForExistingDef() should be refactored out
    // of the parser.
    if (flags & method && !(flags & constructor)) {
        std::istringstream emptyStream;
        Toker toker(emptyStream, name.c_str());
        Parser parser(toker, context);

        Token nameTok(Token::ident, name, new LocationImpl(name.c_str(), 0));

        VarDefPtr existingDef = parser.checkForExistingDef(nameTok, name, true);
        override = parser.checkForOverride(existingDef.get(), realArgs, context->ns.get(), nameTok, name);

        // make overrides implicitly virtual
        if (override) {
            flags = static_cast<Func::Flags>(flags | virtualized);
        }
    }

    // if we have a function pointer, create a extern function for it
    if (funcPtr) {

        // if this is a vwrap, use an internal name for the function so as not 
        // to conflict with the actual virtual function we're creating
        string externName;
        if (flags & vwrap) {
            externName = name + ":vwrap";
        } else if (override) {
            // if this is an override, create a wrapper function
            externName = name + ":impl_" + receiverType->name;

            // this is only the extern function, it can't be virtual.
            // the wrapper for it, further down, is.
            flags = static_cast<Func::Flags>(flags & ~virtualized);
        } else {
            externName = name;
        }

        funcDef =
            builder.createExternFunc(*realCtx,
                                     static_cast<FuncDef::Flags>(flags & 
                                                                 funcDefFlags
                                                                 ),
                                     externName,
                                     returnType->typeDef,
                                     (flags & method) ? receiverType : 0,
                                     realArgs,
                                     funcPtr,
                                     (symbolName.empty()) ? 0 : 
                                      symbolName.c_str()
                                     );

    // inline the function body in the constructor; reduces overhead
    } else if (!funcBody.empty() && !(flags & constructor)) {
        ContextPtr funcContext = context->createSubContext(Context::local);
        funcContext->returnType = returnType->typeDef;
        funcContext->toplevel = true;

        if (flags & method) {
            // create the "this" variable
            ArgDefPtr thisDef =
                context->builder.createArgDef(receiverType, "this");
            funcContext->addDef(thisDef.get());
        }

        funcDef =
            context->builder.emitBeginFunc(*funcContext,
                                            static_cast<FuncDef::Flags>(flags & funcDefFlags),
                                            name,
                                            returnType->typeDef,
                                            realArgs,
                                            override.get()
                                            );

        for (int i = 0; i < realArgs.size(); ++i) {
            funcContext->addDef(realArgs[i].get());
        }

        std::istringstream bodyStream(funcBody);
        Toker toker(bodyStream, name.c_str());
        Parser parser(toker, funcContext.get());
        parser.parse();

        if (returnType->typeDef->matches(*voidType)) {
            funcContext->builder.emitReturn(*funcContext, 0);
        }
        funcContext->builder.emitEndFunc(*funcContext, funcDef.get());
    }

    if (funcDef) {
        VarDefPtr storedDef = context->addDef(funcDef.get(), receiverType);

        // check for a static method, add it to the meta-class
        if (realCtx->scope == Context::instance) {
            TypeDef *type = TypeDefPtr::arcast(realCtx->ns);
            if (type != type->type.get())
                type->type->addAlias(storedDef.get());
        }

        vtableSlot = funcDef->getVTableOffset();
    }

    if (flags & constructor) {
        ContextPtr funcContext = context->createSubContext(Context::local);
        funcContext->toplevel = true;
    
        // create the "this" variable
        ArgDefPtr thisDef =
            context->builder.createArgDef(receiverType, "this");
        funcContext->addDef(thisDef.get());
        VarRefPtr thisRef = funcContext->builder.createVarRef(thisDef.get());
        
        // emit the function
        FuncDefPtr newFunc = context->builder.emitBeginFunc(*funcContext,
                                                            FuncDef::method,
                                                            "oper init",
                                                            voidType,
                                                            realArgs,
                                                            override.get()
                                                            );
        
        // emit the initializers
        Initializers inits;
        if (!ctorInitializers.empty()) {
            std::istringstream initsStream(ctorInitializers);
            Toker initsToker(initsStream, name.c_str());
            Parser initsParser(initsToker, funcContext.get());
            initsParser.parseInitializers(&inits, thisRef.get());
        }
        receiverType->emitInitializers(*funcContext, &inits);
        
        // if we got a function, emit a call to it.
        if (funcDef) {
            FuncCallPtr call = context->builder.createFuncCall(funcDef.get());
            call->receiver = thisRef;
            
            // populate the arg list with references to the existing args
            for (int i = 0; i < realArgs.size(); ++i) {
                VarRefPtr ref =
                    context->builder.createVarRef(realArgs[i].get());
                call->args.push_back(ref.get());
            }
            
            funcContext->createCleanupFrame();
            call->emit(*funcContext)->handleTransient(*funcContext);
            funcContext->closeCleanupFrame();
        } else if (!funcBody.empty()) {
            for (int i = 0; i < realArgs.size(); ++i) {
                funcContext->addDef(realArgs[i].get());
            }

            std::istringstream bodyStream(funcBody);
            Toker toker(bodyStream, name.c_str());
            Parser parser(toker, funcContext.get());
            parser.parse();
        }

        // close it off
        funcContext->builder.emitReturn(*funcContext, 0);
        funcContext->builder.emitEndFunc(*funcContext, newFunc.get());
        context->addDef(newFunc.get(), receiverType);

        receiverType->createNewFunc(*realCtx, newFunc.get());
    
    // is this a virtual wrapper class?
    } else if ((flags & vwrap) || (override && funcPtr)) {
        if (flags & vwrap) {
            assert(wrapperClass && "class wrapper not specified for wrapped func");
        }

        ContextPtr funcContext = context->createSubContext(Context::local);
        funcContext->returnType = returnType->typeDef;
        funcContext->toplevel = true;
    
        // create the "this" variable
        ArgDefPtr thisDef =
            context->builder.createArgDef((flags & vwrap) ? wrapperClass : receiverType, "this");
        funcContext->addDef(thisDef.get());
        VarRefPtr thisRef = funcContext->builder.createVarRef(thisDef.get());
        
        // emit the function
        FuncDef::Flags realFlags = FuncDef::method | FuncDef::virtualized;
        FuncDefPtr newFunc = context->builder.emitBeginFunc(*funcContext,
                                                            realFlags,
                                                            name,
                                                            returnType->typeDef,
                                                            realArgs,
                                                            override.get()
                                                            );

        // copy the args into the context.
        for (int i = 0; i < realArgs.size(); ++i) {
            funcContext->addDef(realArgs[i].get());
        }

        // create a call to the real function        
        FuncCallPtr call = context->builder.createFuncCall(funcDef.get());
        call->receiver = thisRef;
        
        for (int i = 0; i < realArgs.size(); ++i) {
            VarRefPtr ref = 
                funcContext->builder.createVarRef(realArgs[i].get());
            call->args.push_back(ref);
        }

        if (returnType->typeDef != voidType) {
            funcContext->builder.emitReturn(*funcContext, call.get());
        } else {
            call->emit(*funcContext)->handleTransient(*funcContext);
            funcContext->builder.emitReturn(*funcContext, 0);
        }
        funcContext->builder.emitEndFunc(*funcContext, newFunc.get());
        context->addDef(newFunc.get(), (flags & vwrap) ? wrapperClass : receiverType);

        vtableSlot = newFunc->getVTableOffset();
    }

    finished = true;
}
