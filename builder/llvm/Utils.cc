// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "Utils.h"

#include "BTypeDef.h"
#include "Ops.h"
#include "LLVMBuilder.h"
#include "BCleanupFrame.h"

#include <llvm/Module.h>
#include <llvm/LLVMContext.h>
#include <llvm/GlobalValue.h>
#include <llvm/GlobalVariable.h>
#include <spug/StringFmt.h>

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
    arrayAlloc->args[0] = new ArgDef(gd->uintType.get(), "size");
    context.addDef(arrayAlloc.get(), arrayType);
    
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

    // get the LLVM class structure type from out of the meta class rep.
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    PointerType *classPtrType = cast<PointerType>(classType->rep);
    StructType *classStructType =
        cast<StructType>(classPtrType->getElementType());

    // create a global variable holding the class object.
    vector<Constant *> classStructVals(3);

    Constant *zero = ConstantInt::get(Type::getInt32Ty(lctx), 0);
    Constant *index00[] = { zero, zero };

    // build the unique canonical name we need to ensure unique symbols
    // normally type->getFullName() would do this, but at this stage
    // the type itself doesn't have an owner yet, so we build it from
    // context
    string canonicalName(earlyCanonicalize(context, type->name));

    // name
    Constant *nameInit = ConstantArray::get(lctx, type->name, true);
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
        Constant *baseClassPtr = base->classInst;
        
        // make sure that this is in the module
        if (cast<GlobalValue>(baseClassPtr)->getParent() != llvmBuilder.module) {
            const string &name = baseClassPtr->getName();
            Type *classInstType =
                cast<PointerType>(baseClassPtr->getType())->getElementType();
            
            // see if we've already got a copy in this module
            baseClassPtr =
              llvmBuilder.module->getGlobalVariable(baseClassPtr->getName());

            // if not, create a new global variable for us
            if (!baseClassPtr) {
                GlobalVariable *gvar;
                baseClassPtr = gvar =
                    new GlobalVariable(*llvmBuilder.module, 
                                       classInstType,
                                       true,
                                       GlobalValue::ExternalLinkage,
                                       0, // initializer: null for externs
                                       name
                                       );
                llvmBuilder.addGlobalVarMapping(gvar, base->classInst);
            }
        }

        // 1) if the base class is Class, just use the pointer
        // 2) if it's Class[BaseName], GEP our way into the base class 
        // (Class) instance.
        if (base->type.get() == base.get())
            basesVal[i] = baseClassPtr;
        else
            basesVal[i] =
                ConstantExpr::getGetElementPtr(baseClassPtr, index00, 2);
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
    type->classInst =
        new GlobalVariable(*llvmBuilder.module, metaClassStructType,
                           true, // is constant
                           GlobalValue::ExternalLinkage,
                           classObjVal,
                           canonicalName +  ":body"
                           );

    // create the pointer to the class instance
    GlobalVariable *classInstPtr =
        new GlobalVariable(*llvmBuilder.module, metaClassPtrType,
                           true, // is constant
                           GlobalVariable::ExternalLinkage,
                           type->classInst,
                           canonicalName
                           );

    // store the impl object in the class
    type->impl = new BGlobalVarDefImpl(classInstPtr);
}

// Create a new meta-class.
// context: enclosing context.
// name: the original non-canonical class name.
BTypeDefPtr createMetaClass(Context &context, const string &name) {
    LLVMBuilder &llvmBuilder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    LLVMContext &lctx = getGlobalContext();

    // we canonicalize locally here with context because a canonical
    // name for the type is not always available yet
    string canonicalName(earlyCanonicalize(context, name));

    BTypeDefPtr metaType =
        new BTypeDef(context.construct->classType.get(),
                     SPUG_FSTR(canonicalName << ":meta"),
                     0,
                     true,
                     0
                     );
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    metaType->addBaseClass(classType);
    PointerType *classPtrType = cast<PointerType>(classType->rep);
    StructType *classStructType =
        cast<StructType>(classPtrType->getElementType());

    // Create a struct representation of the meta class.  This just has the
    // Class class as its only field.
    vector<Type *> fields(1);
    fields[0] = classStructType;
    StructType *metaClassStructType =
            StructType::create(lctx, fields);
    Type *metaClassPtrType = PointerType::getUnqual(metaClassStructType);
    metaType->rep = metaClassPtrType;
    metaType->complete = true;

    // make the owner the builtin module if it is available, if it is not we 
    // are presumably registering primitives, so just use the module namespace.
    if (context.construct->builtinMod)
        context.construct->builtinMod->addDef(metaType.get());
    else
        context.getDefContext()->addDef(metaType.get());
    createClassImpl(context, metaType.get());
        
    metaClassStructType->setName(metaType->getFullName());

    return metaType;
}

}} // namespace builder::mvll
