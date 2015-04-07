// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModelBuilder.h"

#include "ModelFuncDef.h"

using namespace model;
using namespace builder::mdl;
using namespace std;

namespace {
    FuncDefPtr newFuncDef(TypeDef *returnType, FuncDef::Flags flags,
                          const string &name,
                          size_t argCount
                          ) {
        FuncDefPtr result = new ModelFuncDef(flags, name, argCount);
        result->returnType = returnType;
        return result;
    }

    FuncDefPtr newUnOpDef(TypeDef *returnType, const string &name,
                          bool isMethod
                          ) {
        FuncDefPtr result = newFuncDef(
            returnType,
            FuncDef::builtin |
             (isMethod ? FuncDef::method : FuncDef::noFlags),
            name,
            isMethod ? 0 : 1
        );
        if (isMethod)
            result->receiverType = returnType;
        else
            result->args[0] = new ArgDef(returnType, "operand");
        return result;
    }

    FuncDefPtr newBinOpDef(const string &name, TypeDef *argType,
                           TypeDef *returnType,
                           bool isMethod = false,
                           bool isReversed = false
                           ) {
        FuncDefPtr result = newFuncDef(
            returnType,
            (isMethod ? FuncDef::method : FuncDef::noFlags) |
             (isReversed ? FuncDef::reverse : FuncDef::noFlags) |
             FuncDef::builtin,
            name,
            isMethod ? 1 : 2
            );

        int arg = 0;
        if (isMethod)
            result->receiverType = argType;
        else
            result->args[arg++] = new ArgDef(argType, "lhs");

        result->args[arg++] = new ArgDef(argType, "rhs");
        return result;
    }

    void addNopNew(Context &context, TypeDef *type) {
        FuncDefPtr func = newFuncDef(type, FuncDef::noFlags, "oper new", 1);
        func->args[0] = new ArgDef(type, "val");
        context.addDef(func.get(), type);
    }

    FuncDefPtr newVoidPtrOpDef(TypeDef *resultType) {
        return newUnOpDef(resultType, "oper to .builtin.voidptr", true);
    }

    struct PrimTypeBuilder {
        vector<TypeDef *> deferMetaClass;
        Context &context;
        TypeDef *classType;

        PrimTypeBuilder(Context &context, TypeDef *classType) :
            context(context),
            classType(classType) {
        }

        static ExprPtr makeNullInitializer(TypeDef *type) {
            return new NullConst(type);
        }

        static ExprPtr makeIntInitializer(TypeDef *type) {
            return new IntConst(type, (int64_t)0);
        }

        static ExprPtr makeFloatInitializer(TypeDef *type) {
            return new FloatConst(type, 0);
        }

        TypeDefPtr makeType(const string &name,
                            ExprPtr (*makeInitializer)(TypeDef *type) =
                                makeNullInitializer
                            ) {
            TypeDefPtr result = new TypeDef(classType, "voidptr", false);
            if (makeInitializer)
                result->defaultInitializer = makeInitializer(result.get());
            context.addDef(result.get());
            deferMetaClass.push_back(result.get());
            return result;
        }

        TypeDefPtr makeIntType(const string &name) {
            return makeType(name, makeIntInitializer);
        }

        TypeDefPtr makeFloatType(const string &name) {
            return makeType(name, makeFloatInitializer);
        }
    };
}

ModuleDefPtr ModelBuilder::registerPrimFuncs(Context &context) {
    ModuleDefPtr builtins = new ModelModuleDef(".builtin", 0);
    context.ns = builtins;

    // Class type.
    Construct *gd = context.construct;
    TypeDef *classType;
    gd->classType = classType = new TypeDef(0, "Class", true);
    classType->type = classType;
    classType->meta = classType;
    classType->defaultInitializer = new NullConst(classType);
    context.addDef(classType);

    PrimTypeBuilder typeBuilder(context, classType);

    // void
    TypeDefPtr type;
    gd->voidType = type = typeBuilder.makeType("void", /* makeInitializer */ 0);

    // voidptr
    gd->voidptrType = type = typeBuilder.makeType("voidptr");

    // now that we've got a voidptr type, give the class object a cast to it.
    context.addDef(newVoidPtrOpDef(type.get()).get(), classType);

    TypeDef *byteptrType =
        (gd->byteptrType = type = typeBuilder.makeType("byteptr")).get();
    context.addDef(newVoidPtrOpDef(type.get()).get(), type.get());

    // UNTs
    TypeDef *boolType = (gd->boolType = typeBuilder.makeIntType("bool")).get();
    TypeDef *byteType =
        (gd->byteType = typeBuilder.makeIntType("byte")).get();
    TypeDef *int16Type =
        (gd->int16Type = typeBuilder.makeIntType("int16")).get();
    TypeDef *int32Type =
        (gd->int32Type = typeBuilder.makeIntType("int32")).get();
    TypeDef *int64Type =
        (gd->int64Type = typeBuilder.makeIntType("int64")).get();
    TypeDef *uint16Type =
        (gd->uint16Type = typeBuilder.makeIntType("uint16")).get();
    TypeDef *uint32Type =
        (gd->uint32Type = typeBuilder.makeIntType("uint32")).get();
    TypeDef *uint64Type =
        (gd->uint64Type = typeBuilder.makeIntType("uint64")).get();
    TypeDef *float32Type =
        (gd->float32Type = typeBuilder.makeFloatType("float32")).get();
    TypeDef *float64Type =
        (gd->float64Type = typeBuilder.makeFloatType("float64")).get();

    // PDNTs
    TypeDef *intType = (gd->intType = typeBuilder.makeIntType("int")).get();
    TypeDef *uintType = (gd->uintType = typeBuilder.makeIntType("uint")).get();
    gd->intSize = 32;
    TypeDef *intzType =
        (gd->intzType = typeBuilder.makeIntType("intz")).get();
    TypeDef *uintzType =
        (gd->uintzType = typeBuilder.makeIntType("uintz")).get();
    TypeDef *atomicType = typeBuilder.makeIntType("atomic_int").get();
    TypeDef *floatType =
        (gd->floatType = typeBuilder.makeFloatType("float")).get();
    gd->intzSize = 32;

    // uintz(voidptr)
    FuncDefPtr funcDef = newFuncDef(uintzType, FuncDef::noFlags,
                                    "oper new",
                                    1
                                    );
    funcDef->args[0] = new ArgDef(gd->voidptrType.get(), "val");
    context.addDef(funcDef.get(), uintzType);

#define INTOPS(type, signed, shift, ns) \
    context.addDef(newBinOpDef("oper +", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper -", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper *", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper /", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper %", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper ==", type, boolType, ns).get(), ns);     \
    context.addDef(newBinOpDef("oper !=", type, boolType, ns).get(), ns);     \
    context.addDef(newBinOpDef("oper >", type, boolType, ns).get(), ns);      \
    context.addDef(newBinOpDef("oper <", type, boolType, ns).get(), ns);      \
    context.addDef(newBinOpDef("oper >=", type, boolType, ns).get(), ns);    \
    context.addDef(newBinOpDef("oper <=", type, boolType, ns).get(), ns);     \
    context.addDef(newUnOpDef(type, "oper -", ns).get(), ns);                 \
    context.addDef(newUnOpDef(type, "oper ~", ns).get(), ns);                 \
    context.addDef(newBinOpDef("oepr |", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper &", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper ^", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper <<", type, type, ns).get(), ns);         \
    context.addDef(newBinOpDef("oper >>", type, type, ns).get(), ns);

    INTOPS(byteType, U, L, 0)
    INTOPS(int16Type, S, A, 0)
    INTOPS(uint16Type, U, L, 0)
    INTOPS(int32Type, S, A, 0)
    INTOPS(uint32Type, U, L, 0)
    INTOPS(int64Type, S, A, 0)
    INTOPS(uint64Type, U, L, 0)

    // float operations
#define FLOPS(type, ns) \
    context.addDef(newBinOpDef("oper +", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper -", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper *", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper /", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper %", type, type, ns).get(), ns);          \
    context.addDef(newBinOpDef("oper ==", type, boolType, ns).get(), ns);     \
    context.addDef(newBinOpDef("oper !=", type, boolType, ns).get(), ns);     \
    context.addDef(newBinOpDef("oper >", type, boolType, ns).get(), ns);      \
    context.addDef(newBinOpDef("oper <", type, boolType, ns).get(), ns);      \
    context.addDef(newBinOpDef("oper >=", type, boolType, ns).get(), ns);     \
    context.addDef(newBinOpDef("oper <=", type, boolType, ns).get(), ns);     \
    context.addDef(newUnOpDef(type, "oper -", ns).get(), ns);

    FLOPS(gd->float32Type.get(), 0)
    FLOPS(gd->float64Type.get(), 0)

// Reverse integer operations
#define REVINTOPS(type, signed, shift) \
    context.addDef(newBinOpDef("oper +", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper -", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper *", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper /", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper %", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper ==", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper !=", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper >", type, boolType, true, true).get(),   \
                   type);                                                     \
    context.addDef(newBinOpDef("oper <", type, boolType, true, true).get(),   \
                   type);                                                     \
    context.addDef(newBinOpDef("oper >=", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper <=", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper |", type, 0, true, true).get(), type);   \
    context.addDef(newBinOpDef("oper &", type, 0, true, true).get(), type);   \
    context.addDef(newBinOpDef("oper ^", type, 0, true, true).get(), type);   \
    context.addDef(newBinOpDef("oper <<", type, 0, true, true).get(), type);  \
    context.addDef(newBinOpDef("oper >>", type, 0, true, true).get(), type);

// reverse floating point operations
#define REVFLOPS(type) \
    context.addDef(newBinOpDef("oper +", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper -", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper *", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper /", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper %", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper ==", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper !=", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper >", type, boolType, true, true).get(),   \
                   type);                                                     \
    context.addDef(newBinOpDef("oper <", type, boolType, true, true).get(),   \
                   type);                                                     \
    context.addDef(newBinOpDef("oper >=", type, boolType, true, true).get(),  \
                   type);                                                     \
    context.addDef(newBinOpDef("oper <=", type, boolType, true, true).get(),  \
                   type);

    INTOPS(gd->intType.get(), S, A, gd->intType.get())
    REVINTOPS(gd->intType.get(), S, A)
    INTOPS(gd->uintType.get(), U, L, gd->uintType.get())
    REVINTOPS(gd->uintType.get(), U, L)
    INTOPS(gd->intzType.get(), S, A, gd->intzType.get())
    REVINTOPS(gd->intzType.get(), S, A)
    INTOPS(uintzType, U, L, uintzType)
    REVINTOPS(gd->intType.get(), U, L)
    FLOPS(gd->floatType.get(), gd->floatType.get())
    REVFLOPS(gd->floatType.get())

    context.addDef(newBinOpDef("oper +=", intzType, intzType, true).get(),
                   atomicType
                   );
    context.addDef(newBinOpDef("oper -=", intzType, intzType, true).get(),
                   atomicType
                   );
    context.addDef(newUnOpDef(intzType, "oper to .builtin.intz", true).get(),
                   atomicType
                   );
    context.addDef(newUnOpDef(gd->intType.get(), "oper to .builtin.int",
                              true
                              ).get(),
                   atomicType
                   );
    context.addDef(newUnOpDef(gd->uintType.get(), "oper to .builtin.uint",
                              true
                              ).get(),
                   atomicType
                   );

    // logical operators
    context.addDef(newBinOpDef("oper &&", boolType, boolType).get());
    context.addDef(newBinOpDef("oper ||", boolType, boolType).get());

#define CONVERTER(src, dst) \
    context.addDef(                                                           \
        newUnOpDef(dst##Type, "oper to .builtin." #dst, true).get(),          \
        src##Type                                                             \
    );

    // implicit conversions (no loss of precision)
    CONVERTER(byte, int16);
    CONVERTER(byte, int32);
    CONVERTER(byte, int64);
    CONVERTER(byte, uint16);
    CONVERTER(byte, uint32);
    CONVERTER(byte, uint64);
    CONVERTER(int16, int32);
    CONVERTER(int16, int64);
    CONVERTER(uint16, int32);
    CONVERTER(uint16, int64);
    CONVERTER(uint16, uint32);
    CONVERTER(uint16, uint64);
    CONVERTER(byte, float32);
    CONVERTER(byte, float64);
    CONVERTER(int16, float32);
    CONVERTER(int16, float64);
    CONVERTER(uint16, float32);
    CONVERTER(uint16, float64);
    CONVERTER(int32, int64);
    CONVERTER(int32, float64);
    CONVERTER(uint32, uint64);
    CONVERTER(uint32, int64);
    CONVERTER(uint32, float64);
    CONVERTER(float32, float64);

    CONVERTER(int32, int);
    CONVERTER(uint32, uint);
    CONVERTER(int32, uint);
    CONVERTER(uint32, int);
    CONVERTER(uint32, uint);
    CONVERTER(int64, int);
    CONVERTER(int64, uint);
    CONVERTER(uint64, int);
    CONVERTER(uint64, uint);

    CONVERTER(int32, intz);
    CONVERTER(int32, uintz);
    CONVERTER(uint32, intz);
    CONVERTER(uint32, uintz);
    CONVERTER(int64, intz);
    CONVERTER(int64, uintz);
    CONVERTER(uint64, intz);
    CONVERTER(uint64, uintz);

    CONVERTER(float32, float);
    CONVERTER(float64, float);
    CONVERTER(float, float64);

    CONVERTER(int16, float);
    CONVERTER(uint16, float);
    CONVERTER(int32, float);
    CONVERTER(uint32, float);
    CONVERTER(int64, float);
    CONVERTER(uint64, intz);
    CONVERTER(uint64, uintz);
    CONVERTER(uint64, float);
    CONVERTER(float32, int);
    CONVERTER(float32, uint);
    CONVERTER(float32, intz);
    CONVERTER(float32, uintz);
    CONVERTER(float64, int);
    CONVERTER(float64, uint);
    CONVERTER(float64, intz);
    CONVERTER(float64, uintz);

    CONVERTER(int, int64);
    CONVERTER(uint, uint64);

    CONVERTER(intz, int64);
    CONVERTER(uintz, uint64);

    CONVERTER(float, float64);

    CONVERTER(uint, float32);
    CONVERTER(int, float32);
    CONVERTER(uint, float64);
    CONVERTER(int, float64);
    CONVERTER(uintz, float32);
    CONVERTER(intz, float32);
    CONVERTER(uintz, float64);
    CONVERTER(intz, float64);

    CONVERTER(int, uint);
    CONVERTER(uint, int);
    CONVERTER(intz, uintz);
    CONVERTER(uintz, intz);

    CONVERTER(int, intz);
    CONVERTER(int, uintz);
    CONVERTER(uint, intz);
    CONVERTER(uint, uintz);

    CONVERTER(intz, int);
    CONVERTER(intz, uint);
    CONVERTER(uintz, int);
    CONVERTER(uintz, uint);

    CONVERTER(int, float);
    CONVERTER(uint, float);
    CONVERTER(intz, float);
    CONVERTER(uintz, float);
    CONVERTER(float, int);
    CONVERTER(float, uint);
    CONVERTER(float, intz);
    CONVERTER(float, uintz);

#define UNOPMETH(type, name) \
    context.addDef(newUnOpDef(type##Type, name, true).get(), type##Type);

    UNOPMETH(byte, "oper ++x");
    UNOPMETH(int16, "oper ++x");
    UNOPMETH(uint16, "oper ++x");
    UNOPMETH(int32, "oper ++x");
    UNOPMETH(uint32, "oper ++x");
    UNOPMETH(int64, "oper ++x");
    UNOPMETH(uint64, "oper ++x");
    UNOPMETH(int, "oper ++x");
    UNOPMETH(uint, "oper ++x");
    UNOPMETH(intz, "oper ++x");
    UNOPMETH(uintz, "oper ++x");
    UNOPMETH(byteptr, "oper ++x");

    UNOPMETH(byte, "oper --x");
    UNOPMETH(int16, "oper --x");
    UNOPMETH(uint16, "oper --x");
    UNOPMETH(int32, "oper --x");
    UNOPMETH(uint32, "oper --x");
    UNOPMETH(int64, "oper --x");
    UNOPMETH(uint64, "oper --x");
    UNOPMETH(int, "oper --x");
    UNOPMETH(uint, "oper --x");
    UNOPMETH(intz, "oper --x");
    UNOPMETH(uintz, "oper --x");
    UNOPMETH(byteptr, "oper --x");

    UNOPMETH(byte, "oper x++");
    UNOPMETH(int16, "oper x++");
    UNOPMETH(uint16, "oper x++");
    UNOPMETH(int32, "oper x++");
    UNOPMETH(uint32, "oper x++");
    UNOPMETH(int64, "oper x++");
    UNOPMETH(uint64, "oper x++");
    UNOPMETH(int, "oper x++");
    UNOPMETH(uint, "oper x++");
    UNOPMETH(intz, "oper x++");
    UNOPMETH(uintz, "oper x++");
    UNOPMETH(byteptr, "oper x++");

    UNOPMETH(byte, "oper x--");
    UNOPMETH(int16, "oper x--");
    UNOPMETH(uint16, "oper x--");
    UNOPMETH(int32, "oper x--");
    UNOPMETH(uint32, "oper x--");
    UNOPMETH(int64, "oper x--");
    UNOPMETH(uint64, "oper x--");
    UNOPMETH(int, "oper x--");
    UNOPMETH(uint, "oper x--");
    UNOPMETH(intz, "oper x--");
    UNOPMETH(uintz, "oper x--");
    UNOPMETH(byteptr, "oper x--");

    addNopNew(context, int64Type);
    addNopNew(context, uint64Type);
    addNopNew(context, int32Type);
    addNopNew(context, uint32Type);
    addNopNew(context, int16Type);
    addNopNew(context, uint16Type);
    addNopNew(context, byteType);
    addNopNew(context, float32Type);
    addNopNew(context, float64Type);
    addNopNew(context, intType);
    addNopNew(context, uintType);
    addNopNew(context, intzType);
    addNopNew(context, uintzType);
    addNopNew(context, floatType);

    return builtins;
}

