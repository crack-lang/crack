// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "FunctionTypeDef.h"

#include "model/NullConst.h"
#include "Ops.h"
#include "Utils.h"

#include <iostream>
#include <sstream>

#include <spug/StringFmt.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

FunctionTypeDef::FunctionTypeDef(TypeDef *metaType, const std::string &name,
                                 Type *rep
                                 ) :
    BTypeDef(metaType, name, rep) {

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
    BTypeDef *returnCType = BTypeDefPtr::rcast((*types)[0]);

    // remaining types are function arguments
    std::vector<Type *> fun_args;

    if (arity) {
        for (int i = 1; i < arity+1; ++i) {
            BTypeDef *argCType = BTypeDefPtr::rcast((*types)[i]);
            if (argCType == context.construct->voidType) {
                context.error(context.getLocation(),
                          "Cannot specialize function type with void argument");
            }
            fun_args.push_back(argCType->rep);
        }
    }

    Type *llvmFunType = FunctionType::get(returnCType->rep,
                                          fun_args,
                                          false // XXX isVarArg
                                          );
    Type *llvmFunPtrType = PointerType::get(llvmFunType, 0);

    BTypeDefPtr tempSpec =
            new BTypeDef(type.get(),
                         getSpecializedName(types, false),
                         llvmFunPtrType
                         );
    tempSpec->setOwner(this->owner);
    tempSpec->defaultInitializer = new NullConst(tempSpec.get());

    tempSpec->genericParms = *types;
    tempSpec->templateType = this;
    tempSpec->primitiveGenericSpec = true;

    // This class won't be in the module's namespace, we need to record the
    // its existence so we can fix it later.
    LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
    b.recordOrphanedDef(tempSpec.get());

    // create the implementation (this can be called before the meta-class is
    // initialized, so check for it and defer if it is)
    if (context.construct->classType->complete) {
        createClassImpl(context, tempSpec.get());
        tempSpec->createEmptyOffsetsInitializer(context);
    } else {
        b.deferMetaClass.push_back(tempSpec);
    }

    // Give it an "oper to .builtin.voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        tempSpec.get()
    );

    // and "oper to .builtin.bool".
    context.addDef(
        new BoolOpDef(context.construct->boolType.get(),
                      "oper to .builtin.bool"
                      ),
        tempSpec.get()
    );

    // Give it a specialized "oper call" method, which wraps the
    // call to the function pointer
    FuncDefPtr fptrCall = new FunctionPtrOpDef(returnCType, arity);
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
