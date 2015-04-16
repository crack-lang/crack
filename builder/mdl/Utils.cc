// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Utils.h"

#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/TypeDef.h"

#include "ModelFuncDef.h"

using namespace builder::mdl;
using namespace model;
using namespace std;

namespace builder { namespace mdl { namespace util {

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
                       bool isMethod,
                       bool isReversed
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

// Add an "oper new" that converts from source to target.
void addConvNew(Context &context, TypeDef *source, TypeDef *target) {
    FuncDefPtr func = newFuncDef(target, FuncDef::noFlags, "oper new", 1);
    func->args[0] = new ArgDef(source, "val");
    context.addDef(func.get(), target);
}

void addNopNew(Context &context, TypeDef *type) {
    FuncDefPtr func = newFuncDef(type, FuncDef::noFlags, "oper new", 1);
    func->args[0] = new ArgDef(type, "val");
    context.addDef(func.get(), type);
}

FuncDefPtr newVoidPtrOpDef(TypeDef *resultType) {
    return newUnOpDef(resultType, "oper to .builtin.voidptr", true);
}

}}} // namespace builder::mdl::util