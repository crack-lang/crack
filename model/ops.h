// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ops_h_
#define _model_ops_h_

#include "BinOpDef.h"
#include "FuncCall.h"
#include "OpDef.h"

namespace model {

class NegOpCall : public model::FuncCall {
    public:
        NegOpCall(model::FuncDef *def) : FuncCall(def) {}
        virtual model::ExprPtr foldConstants();
};

class BitNotOpCall : public model::FuncCall {
    public:
        BitNotOpCall(model::FuncDef *def) : FuncCall(def) {}
        virtual model::ExprPtr foldConstants();
};

class FNegOpCall : public model::FuncCall {
    public:
        FNegOpCall(model::FuncDef *def) : FuncCall(def) {}
        virtual model::ExprPtr foldConstants();
};

#define BINOPDF(prefix, op) \
    class prefix##OpCall : public model::FuncCall {                         \
        public:                                                             \
            prefix##OpCall(model::FuncDef *def) :                           \
                FuncCall(def) {                                             \
            }                                                               \
                                                                            \
            virtual model::ExprPtr foldConstants();                         \
    };

// Integer operations.
BINOPDF(Add, "+");
BINOPDF(Sub, "-");
BINOPDF(Mul, "*");
BINOPDF(SDiv, "/");
BINOPDF(UDiv, "/");
BINOPDF(SRem, "%");  // Note: C'99 defines '%' as the remainder, not modulo
BINOPDF(URem, "%");  // the sign is that of the dividend, not divisor.
BINOPDF(Or, "|");
BINOPDF(And, "&");
BINOPDF(Xor, "^");
BINOPDF(Shl, "<<");
BINOPDF(LShr, ">>");
BINOPDF(AShr, ">>");
BINOPDF(AddR, "r+");
BINOPDF(SubR, "r-");
BINOPDF(MulR, "r*");
BINOPDF(SDivR, "r/");
BINOPDF(UDivR, "r/");
BINOPDF(SRemR, "r%");
BINOPDF(URemR, "r%");
BINOPDF(OrR, "r|");
BINOPDF(AndR, "r&");
BINOPDF(XorR, "r^");
BINOPDF(ShlR, "r<<");
BINOPDF(LShrR, "r>>");
BINOPDF(AShrR, "r>>");

// Floating point operations.
BINOPDF(FAdd, "+");
BINOPDF(FSub, "-");
BINOPDF(FMul, "*");
BINOPDF(FDiv, "/");
BINOPDF(FRem, "%");
BINOPDF(FAddR, "r+");
BINOPDF(FSubR, "r-");
BINOPDF(FMulR, "r*");
BINOPDF(FDivR, "r/");
BINOPDF(FRemR, "r%");

// An OpDef for operators that can serve as both functions and methods, e.g.
// arithmetic and bitwise negation.  'OpCall' is the FuncCall class
// instantiated by the createFuncCall() method.
template <class OpCall>
class MixedModeOpDef : public OpDef {
    public:
        MixedModeOpDef(TypeDef *resultType, const std::string &name,
                       bool isMethod = false
                       ) :
            OpDef(resultType,
                  FuncDef::builtin |
                  (isMethod ? FuncDef::method : FuncDef::noFlags),
                  name,
                  isMethod ? 0 : 1
                  ) {
            if (!isMethod)
                args[0] = new ArgDef(resultType, "operand");
        }

    virtual model::FuncCallPtr createFuncCall() {
        return new OpCall(this);
    }
};

// Generic binary operation definition.
template <class OpCall>
class GBinOpDef : public BinOpDef {
    public:
        GBinOpDef(model::TypeDef *argType,
                model::TypeDef *resultType,
                const std::string &name,
                bool isMethod = false,
                bool reversed = false
                ) :
            BinOpDef(argType, resultType, name, isMethod, reversed) {
        }

        virtual FuncCallPtr createFuncCall() {
            return new OpCall(this);
        }
};

}  // namespace model

#endif
