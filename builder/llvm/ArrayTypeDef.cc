// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "ArrayTypeDef.h"

#include "model/NullConst.h"
#include "Ops.h"

#include <spug/StringFmt.h>
#include <llvm/DerivedTypes.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

ArrayTypeDef::ArrayTypeDef(TypeDef *metaType, const std::string &name,
                           Type *rep
                           ) : BTypeDef(metaType, name, rep) {

    defaultInitializer = new NullConst(this);
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
    BTypeDefPtr tempSpec =
            new BTypeDef(type.get(),
                         SPUG_FSTR(name << "[" << parmType->getFullName() <<
                                   "]"
                                   ),
                         llvmType
                         );
    tempSpec->setOwner(this);

    context.addDef(new VoidPtrOpDef(context.construct->voidptrType.get()),
                   tempSpec.get()
                   );

    tempSpec->defaultInitializer = new NullConst(tempSpec.get());

    // create the implementation (this can be called before the meta-class is
    // initialized, so check for it and defer if it is)
    if (context.construct->classType->complete) {
        createClassImpl(context, tempSpec.get());
    } else {
        LLVMBuilder &b = dynamic_cast<LLVMBuilder &>(context.builder);
        b.deferMetaClass.push_back(tempSpec);
    }

    // add all of the methods
    addArrayMethods(context, tempSpec.get(), parmType);
    (*generic)[types] = tempSpec;
    return tempSpec.get();
}
