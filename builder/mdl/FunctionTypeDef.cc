// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "FunctionTypeDef.h"

#include "model/NullConst.h"
#include "Utils.h"

#include <iostream>
#include <sstream>

#include <spug/StringFmt.h>

using namespace llvm;
using namespace model;
using namespace builder::mdl;
using namespace builder::mdl::util;

FunctionTypeDef::FunctionTypeDef(TypeDef *metaType, const std::string &name) :
    TypeDef(metaType, name) {

    defaultInitializer = new NullConst(this);
    generic = new SpecializationCache();

}


// specializations of function types actually create a new type
// object, like array
TypeDefPtr FunctionTypeDef::getSpecialization(Context &context,
                                              TypeVecObj *types
                                              ) {
    // see if it already exists
    TypeDef *spec = findSpecialization(types);
    if (spec)
        return spec;

    // need at least one, the return type
    assert(types->size() >= 1);
    int arity(types->size() - 1);

    // return type is always 0
    TypeDef *returnCType = (*types)[0].get();

    if (arity) {
        for (int i = 1; i < arity + 1; ++i) {
            TypeDef *argCType = (*types)[i].get();
            if (argCType == context.construct->voidType) {
                context.error(context.getLocation(),
                              "Cannot specialize function type with void "
                               "argument"
                              );
            }
        }
    }

    TypeDefPtr tempSpec =
            new TypeDef(type.get(),
                        getSpecializedName(types, false)
                        );
    tempSpec->setOwner(this->owner);
    tempSpec->defaultInitializer = new NullConst(tempSpec.get());

    tempSpec->genericParms = *types;
    tempSpec->templateType = this;
    tempSpec->primitiveGenericSpec = true;

    // Give it an "oper to .builtin.voidptr" method.
    context.addDef(
        newVoidPtrOpDef(context.construct->voidptrType.get()).get(),
        tempSpec.get()
    );

    // and "oper to .builtin.bool".
    context.addDef(
        newUnOpDef(context.construct->boolType.get(),
                   "oper to .builtin.bool",
                   true
                   ).get(),
        tempSpec.get()
    );

    // Give it a specialized "oper call" method, which wraps the
    // call to the function pointer
    FuncDefPtr fptrCall = newFuncDef(returnCType,
                                     FuncDef::builtin | FuncDef::method,
                                     "oper call",
                                     arity
                                     );
    std::ostringstream argName;
    argName << "arg";
    for (int i = 0; i < arity; ++i) {
        argName << i + 1;
        fptrCall->args[i] = new ArgDef((*types)[i + 1].get(), argName.str());
        argName.rdbuf()->pubseekpos(3);
    }
    fptrCall->receiverType = tempSpec;
    context.addDef(fptrCall.get(), tempSpec.get());

    // cache and return
    (*generic)[types] = tempSpec;
    return tempSpec.get();

}
