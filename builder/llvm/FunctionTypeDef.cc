// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "FunctionTypeDef.h"

#include "model/NullConst.h"
#include "Ops.h"

#include <iostream>
#include <sstream>

#include <spug/StringFmt.h>
#include <llvm/DerivedTypes.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

FunctionTypeDef::FunctionTypeDef(TypeDef *metaType, const std::string &name,
                           const Type *rep
                           ) : BTypeDef(metaType, name, rep) {

    defaultInitializer = new NullConst(this);
    generic = new SpecializationCache();

}


// specializations of function types actually create a new type
// object, like array
TypeDef * FunctionTypeDef::getSpecialization(Context &context,
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
    std::vector<const Type*> fun_args;

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

    TypeDefPtr tempSpec =
            new BTypeDef(type.get(),
                         getSpecializedName(types),
                         llvmFunPtrType
                         );
    tempSpec->setOwner(this);
    tempSpec->defaultInitializer = new NullConst(tempSpec.get());

    // Give it an "oper to .builtin.voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        tempSpec.get()
    );

    // Give it a specialized "oper call" method, which wraps the
    // call to the function pointer
    FuncDefPtr fptrCall = new FunctionPtrOpDef(returnCType, arity);
    std::ostringstream argName;
    argName << "arg";
    for (int i = 0; i < arity; ++i) {
        argName << i+1;
        fptrCall->args[i] = new ArgDef((*types)[i+1].get(), argName.str());
        argName.rdbuf()->pubseekpos(3);
    }
    context.addDef(fptrCall.get(), tempSpec.get());

    // cache and return
    (*generic)[types] = tempSpec;
    return tempSpec.get();

}
