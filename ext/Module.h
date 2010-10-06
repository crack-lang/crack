// Copyright 2010 Google Inc.
// Extension library Module class.

#ifndef _crack_ext_Module_h_
#define _crack_ext_Module_h_

#include <vector>

namespace model {
    class Context;
};

namespace crack { namespace ext {

class Func;
class Type;
class ModuleImpl;

class Module {
    friend class Func;

    public:
        enum BuiltinType {
            classType,
            voidType,
            voidPtrType,
            boolType,
            byteptrType,
            byteType,
            int32Type,
            int64Type,
            uint32Type,
            uint64Type,
            intType,
            uintType,
            float32Type,
            float64Type,
            floatType,
            vtableBaseType,
            objectType,
            stringType,
            staticStringType,
            overloadType,
            endSentinel
        };
        
    private:
        model::Context *context;
        Type *builtinTypes[endSentinel];
        std::vector<Func *> funcs;

    public:
        Module(model::Context *context);
        ~Module();
        
        Type *getType(BuiltinType type);
        Type *getType(const char *name);
        Func *addFunc(Type *returnType, const char *name, void *funcPtr);
};


}} // namespace crack::ext

#endif

