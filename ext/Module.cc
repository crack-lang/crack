// Copyright 2010 Google Inc.

#include "Module.h"

#include <string.h>
#include "model/Context.h"
#include "Type.h"
#include "Func.h"

using namespace crack::ext;
using namespace model;

Module::Module(Context *context)  : context(context), finished(false) {
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

#define GET_TYPE(capName, lowerName)                                    \
    Type *Module::get##capName##Type() {                                \
        return builtinTypes[lowerName##Type] ?                          \
            builtinTypes[lowerName##Type] :                             \
            (builtinTypes[lowerName##Type] =                            \
              new Type(context->globalData->lowerName##Type.get()));    \
    }


GET_TYPE(Class, class)
GET_TYPE(Void, void)
GET_TYPE(Voidptr, voidptr)
GET_TYPE(Bool, bool)
GET_TYPE(Byteptr, byteptr)
GET_TYPE(Byte, byte)
GET_TYPE(Int32, int32)
GET_TYPE(Int64, int64)
GET_TYPE(Int, int)
GET_TYPE(Float32, float32)
GET_TYPE(Float64, float64)
GET_TYPE(Float, float)
GET_TYPE(VTableBase, vtableBase)
GET_TYPE(Object, object)
GET_TYPE(String, string)
GET_TYPE(StaticString, staticString)

Type *Module::getType(const char *name) {
    assert(0 && "not implemented");
}

Type *Module::addType(const char *name) {
    TypeMap::iterator i = types.find(name);
    if (i != types.end()) {
        std::cerr << "Type " << name << " already registered!" << std::endl;
        assert(false);
    }

    Type *result;
    types[name] = result = new Type(name, context);
    return result;
}

Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr) {
    assert(!finished && "Attempting to add a function to a finished module.");
    Func *f = new Func(context, returnType, name, funcPtr, Func::noFlags);
    funcs.push_back(f);
    return f;
}

void Module::finish() {
    // no-op if this is already finished
    if (finished)
        return;

    for (int i = 0; i < funcs.size(); ++i)
        funcs[i]->finish();
    finished = true;
}
