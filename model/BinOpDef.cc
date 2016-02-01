// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BinOpDef.h"

using namespace model;
using namespace std;

BinOpDef::BinOpDef(TypeDef *argType,
                   TypeDef *resultType,
                   const string &name,
                   bool isMethod,
                   bool reversed
                   ) :
    OpDef(resultType,
          (isMethod ? FuncDef::method : FuncDef::noFlags) |
           (reversed ? FuncDef::reverse : FuncDef::noFlags) |
           FuncDef::builtin,
          name,
          isMethod ? 1 : 2
          ) {

    int arg = 0;
    if (!isMethod)
        args[arg++] = new ArgDef(argType, "lhs");
    args[arg] = new ArgDef(argType, "rhs");
}
