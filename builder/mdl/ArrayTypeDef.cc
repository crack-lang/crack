// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ArrayTypeDef.h"

#include <spug/StringFmt.h>

#include "model/NullConst.h"

#include "Utils.h"

using namespace llvm;
using namespace model;
using namespace builder::mdl;
using namespace builder::mdl::util;

ArrayTypeDef::ArrayTypeDef(TypeDef *metaType, const std::string &name) :
    TypeDef(metaType, name) {

    defaultInitializer = new NullConst(this);
    generic = new SpecializationCache();
}

namespace {
    void addArrayMethods(Context &context, TypeDef *arrayType,
                         TypeDef *elemType
                         ) {
        Construct *gd = context.construct;
        FuncDefPtr arrayGetItem =
            newFuncDef(elemType, FuncDef::method, "oper []", 1);
        arrayGetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
        context.addDef(arrayGetItem.get(), arrayType);

        arrayGetItem =
            newFuncDef(elemType, FuncDef::method, "oper []", 1);
        arrayGetItem->args[0] = new ArgDef(gd->intType.get(), "index");
        context.addDef(arrayGetItem.get(), arrayType);

        FuncDefPtr arraySetItem =
            newFuncDef(elemType, FuncDef::method, "oper []=", 2);
        arraySetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
        arraySetItem->args[1] = new ArgDef(elemType, "value");
        context.addDef(arraySetItem.get(), arrayType);

        arraySetItem =
            newFuncDef(elemType, FuncDef::method, "oper []=", 2);
        arraySetItem->args[0] = new ArgDef(gd->intType.get(), "index");
        arraySetItem->args[1] = new ArgDef(elemType, "value");
        context.addDef(arraySetItem.get(), arrayType);

        FuncDefPtr arrayOffset =
            newFuncDef(arrayType, FuncDef::method, "oper +", 1);
        arrayOffset->args[0] = new ArgDef(gd->uintType.get(), "offset");
        context.addDef(arrayOffset.get(), arrayType);

        arrayOffset =
            newFuncDef(arrayType, FuncDef::method, "oper +", 1);
        arrayOffset->args[0] = new ArgDef(gd->intType.get(), "offset");
        context.addDef(arrayOffset.get(), arrayType);

        FuncDefPtr arrayAlloc =
            newFuncDef(arrayType, FuncDef::noFlags, "oper new", 1);
        arrayAlloc->args[0] = new ArgDef(gd->uintzType.get(), "size");
        context.addDef(arrayAlloc.get(), arrayType);

        FuncDefPtr arrayFromVoidptr =
            newFuncDef(arrayType, FuncDef::noFlags, "oper new", 1);
        arrayFromVoidptr->args[0] = new ArgDef(gd->voidptrType.get(), "val");
        context.addDef(arrayFromVoidptr.get(), arrayType);

        context.addDef(newUnOpDef(arrayType, "oper ++x", true).get(),
                       arrayType
                       );
        context.addDef(newUnOpDef(arrayType, "oper --x", true).get(),
                       arrayType
                       );
        context.addDef(newUnOpDef(arrayType, "oper x++", true).get(),
                       arrayType
                       );
        context.addDef(newUnOpDef(arrayType, "oper x--", true).get(),
                       arrayType
                       );
    }
}

// specializations of array types actually create a new type
// object.
TypeDefPtr ArrayTypeDef::getSpecialization(Context &context,
                                           TypeVecObj *types
                                           ) {
    // see if it already exists
    TypeDef *spec = findSpecialization(types);
    if (spec)
        return spec;

    // create it.

    assert(types->size() == 1);

    TypeDef *parmType = (*types)[0].get();

    TypeDefPtr tempSpec =
            new TypeDef(type.get(),
                        SPUG_FSTR(name << "[" << parmType->getFullName() <<
                                   "]"
                                  )
                         );
    tempSpec->setOwner(this->owner);

    // Add conversion to voidptr and to bool.
    context.addDef(newVoidPtrOpDef(context.construct->voidptrType.get()).get(),
                   tempSpec.get()
                   );
    context.addDef(
        newUnOpDef(context.construct->boolType.get(),
                   "oper to .builtin.bool",
                   true
                   ).get(),
        tempSpec.get()
    );

    tempSpec->defaultInitializer = new NullConst(tempSpec.get());

    tempSpec->genericParms = *types;
    tempSpec->templateType = this;
    tempSpec->primitiveGenericSpec = true;

    // add all of the methods and finish up.
    addArrayMethods(context, tempSpec.get(), parmType);
    tempSpec->complete = true;
    (*generic)[types] = tempSpec;
    return tempSpec.get();
}
