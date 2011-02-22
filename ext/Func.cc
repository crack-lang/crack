// Copyright 2010 Google Inc.

#include "Func.h"

#include "model/ArgDef.h"
#include "model/CleanupFrame.h"
#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/Initializers.h"
#include "model/ResultExpr.h"
#include "model/VarRef.h"
#include "builder/Builder.h"
#include "Module.h"
#include "Type.h"

using namespace crack::ext;
using namespace std;
using namespace model;
using namespace builder;

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

void Func::finish() {
    if (finished || !context)
        return;

    Builder &builder = context->builder;
    std::vector<ArgDefPtr> realArgs(args.size());
    for (int i = 0; i < args.size(); ++i) {
        args[i]->type->checkFinished();
        realArgs[i] = builder.createArgDef(args[i]->type->typeDef, 
                                           args[i]->name
                                           );
    }

    // if this is a constructor, there may not be a function
    FuncDefPtr funcDef;
    if (funcPtr) {
        
        // if this is a method, get the receiver type
        TypeDefPtr receiverType;
        if (flags & method)
            receiverType = TypeDefPtr::arcast(context->ns);

        funcDef =
            builder.createExternFunc(*context,
                                    static_cast<FuncDef::Flags>(flags & 
                                                                funcDefFlags
                                                                ),
                                    name,
                                    returnType->typeDef,
                                    receiverType.get(),
                                    realArgs,
                                    funcPtr,
                                    (symbolName.empty())?0:symbolName.c_str()
                                    );
        context->addDef(funcDef.get());
    }

    if (flags & constructor) {
        TypeDefPtr myClass = TypeDefPtr::arcast(context->ns);
        ContextPtr funcContext = context->createSubContext(Context::local);
        funcContext->toplevel = true;
    
        // create the "this" variable
        ArgDefPtr thisDef =
            context->builder.createArgDef(myClass.get(), "this");
        funcContext->addDef(thisDef.get());
        VarRefPtr thisRef = new VarRef(thisDef.get());
        
        // emit the function
        TypeDef *voidType = context->construct->voidType.get();
        FuncDefPtr newFunc = context->builder.emitBeginFunc(*funcContext,
                                                            FuncDef::method,
                                                            "oper init",
                                                            voidType,
                                                            realArgs,
                                                            0
                                                            );
        
        // emit the initializers
        Initializers inits;
        myClass->emitInitializers(*funcContext, &inits);

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
        }

        // close it off
        funcContext->builder.emitReturn(*funcContext, 0);
        funcContext->builder.emitEndFunc(*funcContext, newFunc.get());
        context->addDef(newFunc.get(), myClass.get());

        myClass->createNewFunc(*context, newFunc.get());
    }
    
    finished = true;
}
