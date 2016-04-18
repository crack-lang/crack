// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Utils.h"

#include "BTypeDef.h"
#include "Ops.h"
#include "LLVMBuilder.h"
#include "BCleanupFrame.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <spug/StringFmt.h>
#include <spug/check.h>
#include <model/NullConst.h>

using namespace llvm;
using namespace model;
using namespace std;

namespace builder { namespace mvll {

void closeAllCleanupsStatic(Context &context) {
    BCleanupFrame* frame = BCleanupFramePtr::rcast(context.cleanupFrame);
    while (frame) {
        frame->close();
        frame = BCleanupFramePtr::rcast(frame->parent);
    }
}

void addArrayMethods(Context &context, TypeDef *arrayType,
                     BTypeDef *elemType
                     ) {
    Construct *gd = context.construct;

    // Add conversion to voidptr and to bool.
    context.addDef(new VoidPtrOpDef(gd->voidptrType.get()),
                   arrayType
                   );
    context.addDef(
        new BoolOpDef(gd->boolType.get(),
                      "oper to .builtin.bool"
                      ),
        arrayType
    );

    FuncDefPtr arrayGetItem =
            new GeneralOpDef<ArrayGetItemCall>(elemType, FuncDef::method,
                                               "oper []",
                                               1
                                               );
    arrayGetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    context.addDef(arrayGetItem.get(), arrayType);

    arrayGetItem =
            new GeneralOpDef<ArrayGetItemCall>(elemType, FuncDef::method,
                                               "oper []",
                                               1
                                               );
    arrayGetItem->args[0] = new ArgDef(gd->intType.get(), "index");
    context.addDef(arrayGetItem.get(), arrayType);

    FuncDefPtr arraySetItem =
            new GeneralOpDef<ArraySetItemCall>(elemType, FuncDef::method,
                                               "oper []=",
                                               2
                                               );
    arraySetItem->args[0] = new ArgDef(gd->uintType.get(), "index");
    arraySetItem->args[1] = new ArgDef(elemType, "value");
    context.addDef(arraySetItem.get(), arrayType);

    arraySetItem =
            new GeneralOpDef<ArraySetItemCall>(elemType, FuncDef::method,
                                               "oper []=",
                                               2
                                               );
    arraySetItem->args[0] = new ArgDef(gd->intType.get(), "index");
    arraySetItem->args[1] = new ArgDef(elemType, "value");
    context.addDef(arraySetItem.get(), arrayType);

    FuncDefPtr arrayOffset =
            new GeneralOpDef<ArrayOffsetCall>(arrayType, FuncDef::method,
                                              "oper +",
                                              1
                                              );
    arrayOffset->args[0] = new ArgDef(gd->uintType.get(), "offset");
    context.addDef(arrayOffset.get(), arrayType);

    arrayOffset =
            new GeneralOpDef<ArrayOffsetCall>(arrayType, FuncDef::method,
                                              "oper +",
                                              1
                                              );
    arrayOffset->args[0] = new ArgDef(gd->intType.get(), "offset");
    context.addDef(arrayOffset.get(), arrayType);

    arrayOffset =
            new GeneralOpDef<ArrayNegOffsetCall>(arrayType, FuncDef::method,
                                                 "oper -",
                                                 1
                                                 );
    arrayOffset->args[0] = new ArgDef(gd->uintType.get(), "offset");
    context.addDef(arrayOffset.get(), arrayType);

    arrayOffset =
            new GeneralOpDef<ArrayNegOffsetCall>(arrayType, FuncDef::method,
                                                 "oper -",
                                                 1
                                                 );
    arrayOffset->args[0] = new ArgDef(gd->intType.get(), "offset");
    context.addDef(arrayOffset.get(), arrayType);

    FuncDefPtr arrayAlloc =
            new GeneralOpDef<ArrayAllocCall>(arrayType, FuncDef::noFlags,
                                             "oper new",
                                             1
                                             );
    arrayAlloc->args[0] = new ArgDef(gd->uintzType.get(), "size");
    context.addDef(arrayAlloc.get(), arrayType);

    FuncDefPtr arrayFromVoidptr =
        new GeneralOpDef<UnsafeCastCall>(arrayType, FuncDef::noFlags,
                                         "oper new",
                                         1
                                         );
    arrayFromVoidptr->args[0] = new ArgDef(gd->voidptrType.get(), "val");
    context.addDef(arrayFromVoidptr.get(), arrayType);

    context.addDef(new PreIncrPtrOpDef(arrayType, "oper ++x"), arrayType);
    context.addDef(new PreDecrPtrOpDef(arrayType, "oper --x"), arrayType);
    context.addDef(new PostIncrPtrOpDef(arrayType, "oper x++"), arrayType);
    context.addDef(new PostDecrPtrOpDef(arrayType, "oper x--"), arrayType);
}

namespace {
    string earlyCanonicalize(Context &context, const string &name) {
        string canonicalName(context.parent->ns->getNamespaceName());
        if (canonicalName.empty())
            canonicalName = ".builtin";
        canonicalName.append("."+name);
        return canonicalName;
    }
}

// Create a new meta-class.
// context: enclosing context (this should be a class context).
// name: the original non-canonical class name.
BTypeDefPtr createMetaClass(Context &context, const string &name,
                            Namespace *owner
                            ) {
    LLVMBuilder &llvmBuilder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    LLVMContext &lctx = getGlobalContext();

    // we canonicalize locally here with context because a canonical
    // name for the type is not always available yet
    string canonicalName(earlyCanonicalize(context, name));

    string metaTypeName = SPUG_FSTR(canonicalName << ":meta");
    BTypeDefPtr metaType =
        new BTypeDef(context.construct->classType.get(),
                     name + ":meta",
                     0,
                     true,
                     0
                     );
    BTypeDef *classType = BTypeDef::get(context.construct->classType);
    metaType->addBaseClass(classType);
    PointerType *classPtrType = cast<PointerType>(classType->rep);
    StructType *classStructType =
        cast<StructType>(classPtrType->getElementType());
    metaType->defaultInitializer = new NullConst(metaType.get());

    // Create a struct representation of the meta class.  This just has the
    // Class class as its only field.
    StructType *metaClassStructType = LLVMBuilder::getLLVMType(metaTypeName);
    if (!metaClassStructType) {
        vector<Type *> fields(1);
        fields[0] = classStructType;
        metaClassStructType = StructType::create(lctx, fields);
        LLVMBuilder::putLLVMType(metaTypeName, metaClassStructType);
        metaClassStructType->setName(metaTypeName);
    }
    Type *metaClassPtrType = PointerType::getUnqual(metaClassStructType);
    metaType->rep = metaClassPtrType;
    metaType->complete = true;

    if (owner) {
        context.addDef(metaType.get(), owner);
    } else {
        // make the owner the first definition context enclosing the class's
        // context.
        context.parent->getDefContext()->addDef(metaType.get());
    }
    metaType->createClassImpl(context);
    metaType->createBaseOffsets(context);

    return metaType;
}

}} // namespace builder::mvll
