// Copyright 2010 Google Inc.
// Extension library Module class.

#ifndef _crack_ext_Module_h_
#define _crack_ext_Module_h_

#include <map>
#include <string>
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

    private:
        enum BuiltinType {
            classType,
            voidType,
            voidptrType,
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

        model::Context *context;
        Type *builtinTypes[endSentinel];
        std::vector<Func *> funcs;
        typedef std::map<std::string, Type *> TypeMap;
        TypeMap types;
        bool finished;

    public:
        Module(model::Context *context);
        ~Module();

        // functions to get the primitive types
        Type *getClassType();
        Type *getVoidType();
        Type *getVoidptrType();
        Type *getBoolType();
        Type *getByteptrType();
        Type *getByteType();
        Type *getInt32Type();
        Type *getInt64Type();
        Type *getUint32Type();
        Type *getUint64Type();
        Type *getIntType();
        Type *getUintType();
        Type *getFloat32Type();
        Type *getFloat64Type();
        Type *getFloatType();
        Type *getVTableBaseType();
        Type *getObjectType();
        Type *getStringType();
        Type *getStaticStringType();
        Type *getOverloadType();
        
        Type *getType(const char *name);
        Type *addType(const char *name);
        Func *addFunc(Type *returnType, const char *name, void *funcPtr);
        
        /**
         * finish the module (extension init funcs need not call this, it will 
         * be called automatically by the loader).
         */
        void finish();
};


}} // namespace crack::ext

#endif

