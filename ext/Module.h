// Copyright 2010-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Arno Rehn <arno@arnorehn.de>
// Copyright 2011-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// Extension library Module class.

#ifndef _crack_ext_Module_h_
#define _crack_ext_Module_h_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

namespace model {
    class Context;
    class VarDef;
};

namespace crack { namespace ext {

class Func;
class Type;
class ModuleImpl;

class Module {
    friend class Func;
    friend class Type;

    private:
        enum BuiltinType {
            classType,
            voidType,
            voidptrType,
            boolType,
            byteptrType,
            byteType,
            int16Type,
            int32Type,
            int64Type,
            uint16Type,
            uint32Type,
            uint64Type,
            intType,
            uintType,
            intzType,
            uintzType,
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
        std::vector<model::VarDef *> vars;
        typedef std::map<std::string, Type *> TypeMap;
        TypeMap types;
        bool finished;
    protected:
        Type *addTypeWorker(const char *name, size_t instSize, bool forward,
                            bool hasVTable
                            );

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
        Type *getInt16Type();
        Type *getInt32Type();
        Type *getInt64Type();
        Type *getUint16Type();
        Type *getUint32Type();
        Type *getUint64Type();
        Type *getIntType();
        Type *getUintType();
        Type *getUintzType();
        Type *getIntzType();
        Type *getFloat32Type();
        Type *getFloat64Type();
        Type *getFloatType();
        Type *getVTableBaseType();
        Type *getObjectType();
        Type *getStringType();
        Type *getStaticStringType();
        Type *getOverloadType();
        
        Type *getType(const char *name);
        Type *addType(const char *name, size_t instSize, 
                      bool hasVTable = false
                      );
        Type *addForwardType(const char *name, size_t instSize);

        Func *addFunc(Type *returnType, const char *name, void *funcPtr,
                      const char *symbolName=0);
        Func *addFunc(Type *returnType, const char *name, const std::string& body = std::string());
        void addConstant(Type *type, const std::string &name, double val);
        void addConstant(Type *type, const std::string &name, int64_t val);
        void addConstant(Type *type, const std::string &name, int val) {
            addConstant(type, name, static_cast<int64_t>(val));
        }

        void inject(const std::string& code);

        /**
         * finish the module (extension init funcs need not call this, it will 
         * be called automatically by the loader).
         */
        void finish();
};


}} // namespace crack::ext

#endif

