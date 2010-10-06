// Copyright 2010 Google Inc.

#include "Module.h"

#include <string.h>
#include "model/Context.h"
#include "Type.h"
#include "Func.h"

using namespace crack::ext;
using namespace model;

Module::Module(Context *context)  : context(context) {
    memset(builtinTypes, 0, sizeof(builtinTypes));
}

Module::~Module() {
    int i;

    // cleanup the builtin types
    for (i = 0; i < sizeof(builtinTypes) / sizeof(Type *); ++i)
        delete builtinTypes[i];
    
    // cleanup the funcs
    for (i = 0; i < funcs.size(); ++i)
        delete funcs[i];
}

Type *Module::getType(Module::BuiltinType type) {
    
    // check for an overload type before checking the cache array
    if (type > overloadType)
        return 0;
    
    // see if we've got it cached
    if (builtinTypes[type])
        return builtinTypes[type];
    
    TypeDef *typeDef;
    switch (type) {
        case classType:
            typeDef = context->globalData->classType.get();
            break;
        case voidType:
            typeDef = context->globalData->voidType.get();
            break;
        case voidPtrType:
            typeDef = context->globalData->voidPtrType.get();
            break;
        case boolType:
            typeDef = context->globalData->boolType.get();
            break;
        case byteptrType:
            typeDef = context->globalData->byteptrType.get();
            break;
        case byteType:
            typeDef = context->globalData->byteType.get();
            break;
        case int32Type:
            typeDef = context->globalData->int32Type.get();
            break;
        case int64Type:
            typeDef = context->globalData->int64Type.get();
            break;
        case uint32Type:
            typeDef = context->globalData->uint32Type.get();
            break;
        case uint64Type:
            typeDef = context->globalData->uint64Type.get();
            break;
        case intType:
            typeDef = context->globalData->intType.get();
            break;
        case uintType:
            typeDef = context->globalData->uintType.get();
            break;
        case float32Type:
            typeDef = context->globalData->float32Type.get();
            break;
        case float64Type:
            typeDef = context->globalData->float64Type.get();
            break;
        case floatType:
            typeDef = context->globalData->floatType.get();
            break;
        case vtableBaseType:
            typeDef = context->globalData->vtableBaseType.get();
            break;
        case objectType:
            typeDef = context->globalData->objectType.get();
            break;
        case stringType:
            typeDef = context->globalData->stringType.get();
            break;
        case staticStringType:
            typeDef = context->globalData->staticStringType.get();
            break;
        case overloadType:
            typeDef = context->globalData->overloadType.get();
            break;
        
        default:
            assert(0 && "illegal type referenced");
    }
    
    builtinTypes[type] = new Type(typeDef);
    return builtinTypes[type];
}

Type *Module::getType(const char *name) {
    assert(0 && "not implemented");
}

Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr) {
    Func *f = new Func(this, returnType, name, funcPtr);
    funcs.push_back(f);
    return f;
}
