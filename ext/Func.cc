// Copyright 2010 Google Inc.

#include "Func.h"

#include "model/ArgDef.h"
#include "model/Context.h"
#include "model/FuncDef.h"
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
    args.push_back(new Arg(type, name));
}

void Func::finish() {
    Builder &builder = module->context->builder;
    std::vector<ArgDefPtr> realArgs(args.size());
    for (int i = 0; i < args.size(); ++i)
        realArgs[i] = builder.createArgDef(args[i]->type->typeDef, 
                                           args[i]->name
                                           );
    FuncDefPtr funcDef =
        builder.createExternFunc(*module->context,
                                 FuncDef::noFlags,
                                 name,
                                 returnType->typeDef,
                                 realArgs,
                                 funcPtr
                                 );
    module->context->ns->addDef(funcDef.get());
}

