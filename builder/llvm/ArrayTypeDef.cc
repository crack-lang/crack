// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "ArrayTypeDef.h"

#include "Ops.h"

#include <spug/StringFmt.h>
#include <llvm/DerivedTypes.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

ArrayTypeDef::ArrayTypeDef(TypeDef *metaType, const std::string &name,
                           const Type *rep
                           ) :
BTypeDef(metaType, name, rep) {
    generic = new SpecializationCache();
}

// specializations of array types actually create a new type
// object.
TypeDef * ArrayTypeDef::getSpecialization(Context &context,
                                          TypeVecObj *types
                                          ) {
    // see if it already exists
    TypeDef *spec = findSpecialization(types);
    if (spec)
        return spec;

    // create it.

    assert(types->size() == 1);

    BTypeDef *parmType = BTypeDefPtr::rcast((*types)[0]);

    Type *llvmType = PointerType::getUnqual(parmType->rep);
    TypeDefPtr tempSpec =
            new BTypeDef(type.get(),
                         SPUG_FSTR(name << "[" << parmType->getFullName() <<
                                   "]"
                                   ),
                         llvmType
                         );

    tempSpec->addDef(
            new VoidPtrOpDef(context.globalData->voidptrType.get())
            );

    // add all of the methods
    addArrayMethods(context, tempSpec.get(), parmType);
    (*generic)[types] = tempSpec;
    return tempSpec.get();
}
