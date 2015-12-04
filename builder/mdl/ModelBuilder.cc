// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModelBuilder.h"

#include <dlfcn.h>

#include "spug/stlutil.h"
#include "spug/Exception.h"

#include "model/InstVarDef.h"
#include "model/VarDefImpl.h"
#include "ArrayTypeDef.h"
#include "FunctionTypeDef.h"
#include "ModelFuncDef.h"
#include "Utils.h"

using namespace model;
using namespace builder::mdl;
using namespace std;
using namespace util;

namespace {
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
            TypeDefPtr result = new TypeDef(classType, name, false);
            if (makeInitializer)
                result->defaultInitializer = makeInitializer(result.get());
            context.addDef(result.get());
            deferMetaClass.push_back(result.get());
            return result;
        }

        TypeDefPtr makeNumericType(const string &name,
                                   ExprPtr (*makeInitializer)(TypeDef *type)
                                   ) {
            TypeDefPtr result = makeType(name, makeInitializer);
            context.addDef(newUnOpDef(context.construct->boolType.get(),
                                      "oper to .builtin.bool",
                                      true
                                      ).get(),
                           result.get()
                           );
            return result;
        }

        TypeDefPtr makeIntType(const string &name) {
            return makeNumericType(name, makeIntInitializer);
        }

        TypeDefPtr makeFloatType(const string &name) {
            return makeNumericType(name, makeFloatInitializer);
        }
    };

    TypeDefPtr createMetaClass(Context &context, const string &name) {
        TypeDef *classType = context.construct->classType.get();
        TypeDefPtr metaType = new TypeDef(classType, name + ":meta", true);
        metaType->addBaseClass(classType);
        metaType->defaultInitializer = new NullConst(metaType.get());
        metaType->complete = true;
        context.parent->getDefContext()->addDef(metaType.get());
        return metaType;
    }

    // Creates a class for a pointer type.  'context' is the context of the
    // new class.
    TypeDefPtr createClass(Context &context, const string &name) {
        TypeDefPtr metaClass = createMetaClass(context, name);
        TypeDefPtr type = new TypeDef(metaClass.get(), name, true);
        metaClass->meta = type.get();

        FuncDefPtr unsafeCast =
            newFuncDef(type.get(), FuncDef::noFlags, "unsafeCast", 1);
        unsafeCast->args[0] = new ArgDef(context.construct->voidptrType.get(),
                                         "val"
                                         );
        context.addDef(unsafeCast.get(), metaClass.get());
        context.addDef(
            newVoidPtrOpDef(context.construct->voidptrType.get()).get(),
            type.get()
        );
        type->defaultInitializer = new NullConst(type.get());
        return type;
    }

    class ResultExprImpl : public ResultExpr {
        public:
            ResultExprImpl(Expr *sourceExpr) : ResultExpr(sourceExpr) {}
            ResultExprPtr emit(Context &context) {
                return this;
            }
    };

    class VarDefImplImpl : public VarDefImpl {
        private:
            int instSlot;
            VarDef *owner;
        public:

            VarDefImplImpl() : instSlot(-1) {}
            VarDefImplImpl(int instSlot) : instSlot(instSlot) {}

            virtual ResultExprPtr emitRef(Context &context, VarRef *var) {
                return new ResultExprImpl(var);
            }

            virtual ResultExprPtr emitAssignment(Context &context,
                                                AssignExpr *assign
                                                ) {
                return new ResultExprImpl(assign);
            }

            virtual void emitAddr(Context &context, VarRef *var) {
            }

            virtual bool hasInstSlot() const {
                return instSlot != -1;
            }

            virtual int getInstSlot() const {
                return instSlot;
            }

            virtual bool isInstVar() const {
                return hasInstSlot();
            }

    };
}

model::TypeDefPtr ModelBuilder::getFuncType(
    Context &context,
    TypeDef *returnType,
    const vector<ArgDefPtr> &args
) {
    TypeDef::TypeVecObjPtr paramTypes = new TypeDef::TypeVecObj();
    paramTypes->push_back(returnType);
    SPUG_FOR(vector<ArgDefPtr>, arg, args)
        paramTypes->push_back((*arg)->type.get());
    return context.construct->functionType->getSpecialization(context,
                                                              paramTypes.get()
                                                              );
}

TypeDefPtr ModelBuilder::createClassForward(Context &context,
                                            const string &name
                                            ) {
    model::TypeDefPtr result = createClass(context, name);
    result->forward = true;
    return result;
}

FuncDefPtr ModelBuilder::emitBeginFunc(
    Context &context,
    FuncDef::Flags flags,
    const string &name,
    TypeDef *returnType,
    const vector<ArgDefPtr> &args,
    FuncDef *existing
) {
    assert(returnType);
    if (existing) {
        existing->flags = static_cast<FuncDef::Flags>(existing->flags &
                                                      ~FuncDef::forward
                                                      );
        return existing;
    }

    model::FuncDefPtr func = new ModelFuncDef(flags, name, args.size());
    func->args = args;
    func->returnType = returnType;
    func->type = getFuncType(context, returnType, args);
    return func;
}

model::TypeDefPtr ModelBuilder::emitBeginClass(Context &context,
                                               const string &name,
                                               const vector<TypeDefPtr> &bases,
                                               TypeDef *forwardDef
                                               ) {
    TypeDefPtr result;
    if (forwardDef)
        result = forwardDef;
    else
        result = createClass(context, name);
    result->parents = bases;
    result->fieldCount = bases.size();
    context.ns = result;
    return result;
}

VarDefPtr ModelBuilder::emitVarDef(
    Context &container,
    TypeDef *type,
    const string &name,
    Expr *initializer,
    bool staticScope
) {
    VarDefPtr result;
    ContextPtr defCtx = container.getDefContext();
    if (defCtx->scope == Context::instance) {
        TypeDef *ownerType = TypeDefPtr::arcast(defCtx->ns);
        result =  new InstVarDef(type, name,
                                 initializer ? initializer :
                                               type->defaultInitializer.get()
                                 );
        result->impl = new VarDefImplImpl(ownerType->fieldCount++);
    } else {
        result = new VarDef(type, name);
        result->impl = new VarDefImplImpl();
    }
    return result;
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
    TypeDef *boolType =
        (gd->boolType = typeBuilder.makeType(
            "bool",
            PrimTypeBuilder::makeIntInitializer
        )).get();
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
    context.addDef(newBinOpDef("oper |", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper &", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper ^", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper <<", type, type, true, true).get(), type);\
    context.addDef(newBinOpDef("oper >>", type, type, true, true).get(), type);

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

    // These new operators truncate.
    addConvNew(context, int64Type, uint64Type);
    addConvNew(context, int64Type, int32Type);
    addConvNew(context, int64Type, uint32Type);
    addConvNew(context, int64Type, int16Type);
    addConvNew(context, int64Type, uint16Type);
    addConvNew(context, int64Type, byteType);

    addConvNew(context, uint64Type, int64Type);
    addConvNew(context, uint64Type, int32Type);
    addConvNew(context, uint64Type, uint32Type);
    addConvNew(context, uint64Type, int16Type);
    addConvNew(context, uint64Type, uint16Type);
    addConvNew(context, uint64Type, byteType);

    addConvNew(context, int32Type, byteType);
    addConvNew(context, int32Type, uint16Type);
    addConvNew(context, int32Type, int16Type);
    addConvNew(context, int32Type, uint32Type);

    addConvNew(context, uint32Type, byteType);
    addConvNew(context, uint32Type, int16Type);
    addConvNew(context, uint32Type, uint16Type);
    addConvNew(context, uint32Type, int32Type);

    addConvNew(context, int16Type, byteType);
    addConvNew(context, int16Type, uint16Type);

    addConvNew(context, uint16Type, byteType);
    addConvNew(context, uint16Type, int16Type);

    addConvNew(context, intType, int16Type);
    addConvNew(context, intType, uint16Type);
    addConvNew(context, intType, int32Type);
    addConvNew(context, intType, uint32Type);
    addConvNew(context, intType, byteType);

    addConvNew(context, uintType, int16Type);
    addConvNew(context, uintType, uint16Type);
    addConvNew(context, uintType, int32Type);
    addConvNew(context, uintType, uint32Type);
    addConvNew(context, uintType, byteType);

    addConvNew(context, float64Type, float32Type);
    addConvNew(context, float32Type, byteType);
    addConvNew(context, float32Type, int16Type);
    addConvNew(context, float32Type, uint16Type);
    addConvNew(context, float32Type, int32Type);
    addConvNew(context, float32Type, uint32Type);
    addConvNew(context, float32Type, int64Type);
    addConvNew(context, float32Type, uint64Type);
    addConvNew(context, float64Type, byteType);
    addConvNew(context, float64Type, int16Type);
    addConvNew(context, float64Type, uint16Type);
    addConvNew(context, float64Type, int32Type);
    addConvNew(context, float64Type, uint32Type);
    addConvNew(context, float64Type, int64Type);
    addConvNew(context, float64Type, uint64Type);

    addConvNew(context, int64Type, float32Type);
    addConvNew(context, uint64Type, float32Type);
    addConvNew(context, int64Type, float64Type);
    addConvNew(context, uint64Type, float64Type);

    // create the array generic
    TypeDefPtr arrayType = new ArrayTypeDef(context.construct->classType.get(),
                                            "array"
                                            );
    gd->arrayType = arrayType;
    context.addDef(arrayType.get());
    typeBuilder.deferMetaClass.push_back(arrayType.get());

    TypeDefPtr functionType = new FunctionTypeDef(context.construct->classType.get(),
                                                  "function"
                                                  );
    gd->functionType = functionType;
    context.addDef(functionType.get());
    typeBuilder.deferMetaClass.push_back(functionType.get());

    context.addDef(newBinOpDef("oper is", gd->voidptrType.get(),
                               boolType).get()
                   );
    context.addDef(newBinOpDef("oper is", byteptrType, boolType).get());
    context.addDef(newBinOpDef("oper is", classType, boolType).get());

    // Create OverloadDef.
    TypeDefPtr metaType = createMetaClass(context, "Overload");
    TypeDefPtr overloadDef = new TypeDef(metaType.get(), "Overload", false);
    metaType->meta = overloadDef.get();

    context.addDef(
        newVoidPtrOpDef(gd->voidptrType.get()).get(),
        overloadDef.get()
    );
    context.addDef(overloadDef.get());

    // Create VTableBase.
    metaType = createMetaClass(context, "VTableBase");
    TypeDef *vtableBaseType;
    gd->vtableBaseType = vtableBaseType =
        new TypeDef(metaType.get(), "VTableBase", true);
    vtableBaseType->hasVTable = true;
    vtableBaseType->defaultInitializer = new NullConst(vtableBaseType);
    metaType->meta = vtableBaseType;
    context.addDef(vtableBaseType);
    context.construct->registerDef(vtableBaseType);
    ContextPtr classCtx =
        context.createSubContext(Context::instance, vtableBaseType);
    vtableBaseType->createOperClass(*classCtx);

    context.addDef(newVoidPtrOpDef(vtableBaseType).get());
    vtableBaseType->complete = true;

    context.addDef(newUnOpDef(boolType, "oper !", false).get());

    return builtins;
}

void *ModelBuilder::loadSharedLibrary(const std::string &name) {
    void *handle = dlopen(name.c_str(), RTLD_LAZY|RTLD_GLOBAL);
    if (!handle)
        throw spug::Exception(dlerror());
    return handle;
}
