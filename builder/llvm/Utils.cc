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

void createClassImpl(Context &context, BTypeDef *type) {

    LLVMContext &lctx = getGlobalContext();
    LLVMBuilder &llvmBuilder = dynamic_cast<LLVMBuilder &>(context.builder);

    // in situations where we haven't defined the class body yet, create a 
    // fake class containing the parents so we can compute the base class 
    // offsets.
    PointerType *pointerType = 
        type->rep ? dyn_cast<PointerType>(type->rep) : 0;
    StructType *instType = 
        pointerType ? dyn_cast<StructType>(pointerType->getElementType()) : 0;
    if (!instType || instType->isOpaque()) {
        instType = StructType::create(lctx);
        vector<Type *> fields(type->parents.size());
        for (int i = 0; i < type->parents.size(); ++i) {
            PointerType *basePtrType = 
                cast<PointerType>(BTypeDefPtr::rcast(type->parents[i])->rep);
            fields[i] = basePtrType->getElementType();
        }
        instType->setBody(fields);
        pointerType = instType->getPointerTo();
    }

    Type *intzType = BTypeDefPtr::rcast(context.construct->intzType)->rep,
    
         // GEP offsets have to be int32.  int64 offsets are considered to be
         // "invalid indeces" by validIndex().
         *int32Type = Type::getInt32Ty(lctx);

    // get the LLVM class structure type from out of the meta class rep.
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    PointerType *classPtrType = cast<PointerType>(classType->rep);
    StructType *classStructType =
        cast<StructType>(classPtrType->getElementType());

    // create a global variable holding the class object.
    vector<Constant *> classStructVals(6);

    Constant *zero = ConstantInt::get(int32Type, 0);
    Constant *index00[] = { zero, zero };

    // build the unique canonical name, we need to ensure unique symbols.
    // if the type has an owner, type->getFullName() will do this, but at 
    // this stage the type itself usually doesn't have an owner yet, so we 
    // build it from context.
    string canonicalName(type->getOwner() ?
                            type->getFullName() : 
                            earlyCanonicalize(context, type->name)
                         );

    // name
    Constant *nameInit = ConstantDataArray::getString(lctx, type->name, true);
    GlobalVariable *nameGVar =
            new GlobalVariable(*llvmBuilder.module,
                               nameInit->getType(),
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               nameInit,
                               canonicalName + ":name"
                               );
    classStructVals[0] =
            ConstantExpr::getGetElementPtr(nameGVar, index00, 2);

    // numBases
    Type *uintType = BTypeDefPtr::arcast(context.construct->uintType)->rep;
    classStructVals[1] = ConstantInt::get(uintType, type->parents.size());

    // bases
    vector<Constant *> basesVal(type->parents.size());
    for (int i = 0; i < type->parents.size(); ++i) {
        // get the pointer to the inner "Class" object of "Class[BaseName]"
        BTypeDefPtr base = BTypeDefPtr::arcast(type->parents[i]);

        // get the class body global variable
        Constant *baseClassPtr = 
            base->getClassInstRep(llvmBuilder.moduleDef.get());
        
        // 1) if the base class is Class, just use the pointer
        // 2) if it's Class[BaseName], GEP our way into the base class 
        // (Class) instance.
        if (base->type.get() == base.get())
            basesVal[i] = baseClassPtr;
        else
            basesVal[i] =
                ConstantExpr::getGetElementPtr(baseClassPtr, index00, 2);
        
        SPUG_CHECK(basesVal[i]->getType() == classType->rep,
                   "Base " << i << " of class " << type->getFullName() <<
                    " has an LLVM type that is not that of Class"
                   );
    }
    ArrayType *baseArrayType =
        ArrayType::get(classType->rep, type->parents.size());
    Constant *baseArrayInit = ConstantArray::get(baseArrayType, basesVal);
    GlobalVariable *basesGVar =
        new GlobalVariable(*llvmBuilder.module,
                           baseArrayType,
                           true, // is constant
                           GlobalValue::ExternalLinkage,
                           baseArrayInit,
                           canonicalName +  ":bases"
                           );
    classStructVals[2] =
        ConstantExpr::getGetElementPtr(basesGVar, index00, 2);

    // see if the offsets variable already exists (it can in the case of 
    // .builtin.Class)
    GlobalVariable *offsetsGVar =
        llvmBuilder.module->getGlobalVariable(canonicalName +  ":offsets");

    // The offsets initializer gets created in BTypeDef::fixIncompletes() 
    // after the base class sizes must be known.
    if (!offsetsGVar)
        offsetsGVar =
            new GlobalVariable(*llvmBuilder.module,
                               ArrayType::get(intzType, type->parents.size()),
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               NULL, // initializer, filled in later.
                               canonicalName +  ":offsets"
                               );
    classStructVals[3] =
        ConstantExpr::getGetElementPtr(offsetsGVar, index00, 2);

    // Create numVTables.
    classStructVals[4] =
        ConstantInt::get(uintType, type->countRootAncestors() * 2);

    // Create the vtables array.  This is an array of vtable address, offset
    // from the beginning of the instances.
    Type *intPtrType =
        Type::getInt8Ty(getGlobalContext())->getPointerTo();
    Type *voidptrArrayType =
        ArrayType::get(intPtrType,
                       type == context.construct->vtableBaseType.get() ? 2 :
                         type->countRootAncestors() * 2
                       );
    if (type->hasVTable) {
        GlobalVariable *vtablesGVar =
            new GlobalVariable(*llvmBuilder.module,
                               voidptrArrayType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               NULL, // initializer, filled in later.
                               canonicalName + ":vtables"
                               );
        classStructVals[5] =
            ConstantExpr::getGetElementPtr(vtablesGVar, index00, 2);
    } else {
        // If there are no vtables, just initialize the vtables array to null.
        classStructVals[5] =
            Constant::getNullValue(intPtrType->getPointerTo());
    }
    
    // build the instance of Class
    Constant *classStruct =
        ConstantStruct::get(classStructType, classStructVals);

    // extract the meta class structure types from our meta-class
    PointerType *metaClassPtrType =
        cast<PointerType>(BTypeDefPtr::arcast(type->type)->rep);
    StructType *metaClassStructType =
        cast<StructType>(metaClassPtrType->getElementType());

    // initialize the structure based on whether we're implementing an 
    // instance of Class or an instance of a meta-class derived from Class.
    Constant *classObjVal;
    if (type->type.get() != context.construct->classType.get()) {
        
        // this is an instance of a normal meta-class, wrap the Class 
        // implementation in another structure.
        vector<Constant *> metaClassStructVals(1);
        metaClassStructVals[0] = classStruct;
        classObjVal =
            ConstantStruct::get(metaClassStructType, metaClassStructVals);
    } else {
        
        // this is an instance of Class - just use the Class implementation as 
        // is.
        metaClassPtrType = classPtrType;
        metaClassStructType = classStructType;
        classObjVal = classStruct;
    }

    // Create the class global variable
    GlobalVariable *classInst =
        type->createClassInst(llvmBuilder.moduleDef.get(),
                            metaClassStructType,
                            classObjVal,
                            canonicalName
                            );

    // create the pointer to the class instance
    GlobalVariable *classInstPtr =
        new GlobalVariable(*llvmBuilder.module, metaClassPtrType,
                           true, // is constant
                           GlobalVariable::ExternalLinkage,
                           classInst,
                           canonicalName
                           );

    // store the impl object in the class
    type->impl = new BGlobalVarDefImpl(classInstPtr, 
                                       llvmBuilder.moduleDef->repId
                                       );
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
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
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
    createClassImpl(context, metaType.get());
    metaType->createBaseOffsets(context);

    return metaType;
}

}} // namespace builder::mvll
