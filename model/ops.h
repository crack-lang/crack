// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ops_h_
#define _model_ops_h_

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


}  // namespace model

#endif
