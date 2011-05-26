// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Ops_h_
#define _builder_llvm_Ops_h_

#include "model/FuncCall.h"
#include "model/FuncDef.h"
#include "model/Context.h"
#include "model/ResultExpr.h"

namespace model {
    class TypeDef;
}

namespace builder {
namespace mvll {

class BTypeDef;

// primitive operations
SPUG_RCPTR(OpDef);

class OpDef : public model::FuncDef {
    public:
        
        OpDef(model::TypeDef *resultType, model::FuncDef::Flags flags,
              const std::string &name,
              size_t argCount
              ) :
            FuncDef(flags, name, argCount) {
            
            // XXX we don't have a function type for these
            returnType = resultType;
        }
        
        virtual model::FuncCallPtr createFuncCall() = 0;

        virtual void *getFuncAddr(Builder &builder) {
            return 0;
        }
};

class BinOpDef : public OpDef {
    public:
        BinOpDef(model::TypeDef *argType,
                 model::TypeDef *resultType,
                 const std::string &name);

        virtual model::FuncCallPtr createFuncCall() = 0;
};

class UnOpDef : public OpDef {
    public:
    UnOpDef(model::TypeDef *resultType, const std::string &name) :
            OpDef(resultType, model::FuncDef::method, name, 0) {
        }
};

// No-op call returns its receiver or argument.  This is used for oper new's 
// to avoid doing any conversions if the type is already correct.
class NoOpCall : public model::FuncCall {
public:
    NoOpCall(model::FuncDef *def) : model::FuncCall(def) {}
    
    virtual model::ResultExprPtr emit(model::Context &context);
};

class BitNotOpCall : public model::FuncCall {
public:
    BitNotOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class BitNotOpDef : public OpDef {
public:
    BitNotOpDef(BTypeDef *resultType, const std::string &name);

    virtual model::FuncCallPtr createFuncCall() {
        return new BitNotOpCall(this);
    }

};

class LogicAndOpCall : public model::FuncCall {
public:
    LogicAndOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class LogicAndOpDef : public BinOpDef {
public:
    LogicAndOpDef(model::TypeDef *argType, model::TypeDef *resultType) :
            BinOpDef(argType, resultType, "oper &&") {
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new LogicAndOpCall(this);
    }
};

class LogicOrOpCall : public model::FuncCall {
public:
    LogicOrOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class LogicOrOpDef : public BinOpDef {
public:
    LogicOrOpDef(model::TypeDef *argType, model::TypeDef *resultType) :
            BinOpDef(argType, resultType, "oper ||") {
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new LogicOrOpCall(this);
    }
};

class NegOpCall : public model::FuncCall {
public:
    NegOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class NegOpDef : public OpDef {
public:
    NegOpDef(BTypeDef *resultType, const std::string &name);

    virtual model::FuncCallPtr createFuncCall() {
        return new NegOpCall(this);
    }
};

class FNegOpCall : public model::FuncCall {
public:
    FNegOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class FNegOpDef : public OpDef {
public:
    FNegOpDef(BTypeDef *resultType, const std::string &name);

    virtual model::FuncCallPtr createFuncCall() {
        return new FNegOpCall(this);
    }
};

class FunctionPtrOpDef : public OpDef {
public:
    FunctionPtrOpDef(model::TypeDef *resultType,
                     size_t argCount) :
    OpDef(resultType, FuncDef::method, "oper call", argCount) {
        type = resultType;
    }

    virtual model::FuncCallPtr createFuncCall();

};

class FunctionPtrCall : public model::FuncCall {
public:
    FunctionPtrCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);

    virtual bool isProductive() const { return true; }
};

template<class T>
class GeneralOpDef : public OpDef {
public:
    GeneralOpDef(model::TypeDef *resultType, model::FuncDef::Flags flags,
                 const std::string &name,
                 size_t argCount
                 ) :
    OpDef(resultType, flags, name, argCount) {
        type = resultType;
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new T(this);
    }
};

class NoOpDef : public GeneralOpDef<NoOpCall> {
    public:
        NoOpDef(model::TypeDef *resultType, const std::string &name) : 
            GeneralOpDef<NoOpCall>(resultType, model::FuncDef::method, name,
                                   0
                                   ) {
        }
};

class ArrayGetItemCall : public model::FuncCall {
public:
    ArrayGetItemCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);

    virtual bool isProductive() const { return false; }
};

class ArraySetItemCall : public model::FuncCall {
public:
    ArraySetItemCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);

    virtual bool isProductive() const { return false; }
};

class ArrayAllocCall : public model::FuncCall {
public:
    ArrayAllocCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

// implements pointer arithmetic
class ArrayOffsetCall : public model::FuncCall {
public:
    ArrayOffsetCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

/** Operator to convert simple types to booleans. */
class BoolOpCall : public model::FuncCall {
public:
    BoolOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class BoolOpDef : public UnOpDef {
public:
    BoolOpDef(model::TypeDef *resultType, const std::string &name) :
            UnOpDef(resultType, name) {
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new BoolOpCall(this);
    }
};

class FBoolOpCall : public model::FuncCall {
public:
    FBoolOpCall(model::FuncDef *def) : FuncCall(def) {}

    virtual model::ResultExprPtr emit(model::Context &context);
};

class FBoolOpDef : public UnOpDef {
public:
    FBoolOpDef(model::TypeDef *resultType, const std::string &name) :
            UnOpDef(resultType, name) {
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new FBoolOpCall(this);
    }
};

/** Operator to convert any pointer type to void. */
class VoidPtrOpCall : public model::FuncCall {
public:
    VoidPtrOpCall(model::FuncDef *def) : FuncCall(def) {}
    virtual model::ResultExprPtr emit(model::Context &context);
};

/** Operator to convert a pointer to an integer. */
class PtrToIntOpCall : public model::FuncCall {
public:
    PtrToIntOpCall(model::FuncDef *def) : FuncCall(def) {}
    virtual model::ResultExprPtr emit(model::Context &context);
};

class VoidPtrOpDef : public UnOpDef {
public:
    VoidPtrOpDef(model::TypeDef *resultType) :
            UnOpDef(resultType, "oper to voidptr") {
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new VoidPtrOpCall(this);
    }
};

class UnsafeCastCall : public model::FuncCall {
public:
    UnsafeCastCall(model::FuncDef *def) :
            FuncCall(def) {
    }

    virtual model::ResultExprPtr emit(model::Context &context);

    virtual bool isProductive() const {
        return false;
    }
};

class UnsafeCastDef : public OpDef {
public:
    UnsafeCastDef(model::TypeDef *resultType);

    // Override "matches()" so that the function matches any single
    // argument call.
    virtual bool matches(model::Context &context,
                         const std::vector< model::ExprPtr > &vals,
                         std::vector< model::ExprPtr > &newVals,
                         model::FuncDef::Convert convertFlag
                         ) {
        if (vals.size() != 1)
            return false;

        if (convert)
            newVals = vals;
        return true;
    }

    virtual model::FuncCallPtr createFuncCall() {
        return new UnsafeCastCall(this);
    }
};

#define UNOP_DEF(opCode) \
    class opCode##OpCall : public model::FuncCall {                         \
        public:                                                             \
            opCode##OpCall(model::FuncDef *def) : model::FuncCall(def) {}   \
                                                                            \
            virtual model::ResultExprPtr emit(model::Context &context);     \
    };                                                                      \
                                                                            \
    class opCode##OpDef : public UnOpDef {                                  \
        public:                                                             \
            opCode##OpDef(model::TypeDef *resultType,                       \
                          const std::string &name) :                        \
                UnOpDef(resultType, name) {                                 \
            }                                                               \
                                                                            \
            virtual model::FuncCallPtr createFuncCall() {                   \
                return new opCode##OpCall(this);                            \
            }                                                               \
    };

#define QUAL_BINOP_DEF(prefix, opCode, op)                                  \
    class prefix##OpCall : public model::FuncCall {                         \
        public:                                                             \
            prefix##OpCall(model::FuncDef *def) :                           \
                FuncCall(def) {                                             \
            }                                                               \
                                                                            \
            virtual model::ResultExprPtr emit(model::Context &context);     \
    };                                                                      \
                                                                            \
    class prefix##OpDef : public BinOpDef {                                 \
        public:                                                             \
            prefix##OpDef(model::TypeDef *argType,                          \
                          model::TypeDef *resultType = 0) :                 \
                BinOpDef(argType, resultType ? resultType : argType,        \
                         "oper " op                                         \
                         ) {                                                \
            }                                                               \
                                                                            \
            virtual model::FuncCallPtr createFuncCall() {                   \
                return new prefix##OpCall(this);                            \
            }                                                               \
    };

#define BINOP_DEF(opCode, op) QUAL_BINOP_DEF(opCode, opCode, op)

// Binary Ops
BINOP_DEF(Add, "+");
BINOP_DEF(Sub, "-");
BINOP_DEF(Mul, "*");
BINOP_DEF(SDiv, "/");
BINOP_DEF(UDiv, "/");
BINOP_DEF(SRem, "%");  // Note: C'99 defines '%' as the remainder, not modulo
BINOP_DEF(URem, "%");  // the sign is that of the dividend, not divisor.
BINOP_DEF(Or, "|");
BINOP_DEF(And, "&");
BINOP_DEF(Xor, "^");
BINOP_DEF(Shl, "<<");
BINOP_DEF(LShr, ">>");
BINOP_DEF(AShr, ">>");

BINOP_DEF(ICmpEQ, "==");
BINOP_DEF(ICmpNE, "!=");
BINOP_DEF(ICmpSGT, ">");
BINOP_DEF(ICmpSLT, "<");
BINOP_DEF(ICmpSGE, ">=");
BINOP_DEF(ICmpSLE, "<=");
BINOP_DEF(ICmpUGT, ">");
BINOP_DEF(ICmpULT, "<");
BINOP_DEF(ICmpUGE, ">=");
BINOP_DEF(ICmpULE, "<=");

BINOP_DEF(FAdd, "+");
BINOP_DEF(FSub, "-");
BINOP_DEF(FMul, "*");
BINOP_DEF(FDiv, "/");
BINOP_DEF(FRem, "%");

BINOP_DEF(FCmpOEQ, "==");
BINOP_DEF(FCmpONE, "!=");
BINOP_DEF(FCmpOGT, ">");
BINOP_DEF(FCmpOLT, "<");
BINOP_DEF(FCmpOGE, ">=");
BINOP_DEF(FCmpOLE, "<=");

QUAL_BINOP_DEF(Is, ICmpEQ, "is");

// Type Conversion Ops
UNOP_DEF(SExt);
UNOP_DEF(ZExt);
UNOP_DEF(FPExt);
UNOP_DEF(SIToFP);
UNOP_DEF(UIToFP);
UNOP_DEF(Trunc);
UNOP_DEF(PreIncrInt);
UNOP_DEF(PreDecrInt);
UNOP_DEF(PostIncrInt);
UNOP_DEF(PostDecrInt);

#define FPTRUNCOP_DEF(opCode) \
    class opCode##OpCall : public model::FuncCall {                         \
        public:                                                             \
            opCode##OpCall(model::FuncDef *def) : FuncCall(def) {}          \
                                                                            \
            virtual model::ResultExprPtr emit(model::Context &context);     \
    };                                                                      \

// Floating Point Truncating Ops
FPTRUNCOP_DEF(FPTrunc);
FPTRUNCOP_DEF(FPToSI);
FPTRUNCOP_DEF(FPToUI);

// define a floating point truncation definition so we can use this as either 
// an explicit constructor or an implicit "oper to" for converting to the 
// 'float' PDNT.
class FPTruncOpDef : public UnOpDef {
    public:
        FPTruncOpDef(model::TypeDef *resultType, const std::string &name) :
                UnOpDef(resultType, name) {
        }
    
        virtual model::FuncCallPtr createFuncCall() {
            return new FPTruncOpCall(this);
        }
};

} // end namespace builder::vmll
} // end namespace builder

#endif
