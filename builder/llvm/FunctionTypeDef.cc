// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "FunctionTypeDef.h"

#include "model/NullConst.h"
#include "Ops.h"

#include <spug/StringFmt.h>
#include <llvm/DerivedTypes.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

FunctionTypeDef::FunctionTypeDef(TypeDef *metaType, const std::string &name,
                           const Type *rep
                           ) : BTypeDef(metaType, name, rep) {

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

    // return type is always 0
    BTypeDef *returnCType = BTypeDefPtr::rcast((*types)[0]);

    // remaining types are function arguments
    std::vector<const Type*> fun_args;

    if (types->size() > 1) {
        for (int i = 1; i < types->size(); ++i) {
            BTypeDef *argCType = BTypeDefPtr::rcast((*types)[i]);
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

    // Give it an "oper to voidptr" method.
    context.addDef(
        new VoidPtrOpDef(context.construct->voidptrType.get()),
        tempSpec.get()
    );

    // Give it an

    (*generic)[types] = tempSpec;
    return tempSpec.get();

}
