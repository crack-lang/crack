// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BTypeDef.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include "spug/stlutil.h"
#include "model/Context.h"
#include "model/OverloadDef.h"
#include "BFuncDef.h"
#include "PlaceholderInstruction.h"
#include "VarDefs.h"
#include "VTableBuilder.h"

using namespace model;
using namespace std;
using namespace builder::mvll;
using namespace llvm;

void BTypeDef::getDependents(std::vector<TypeDefPtr> &deps) {
    for (IncompleteChildVec::iterator iter = incompleteChildren.begin();
         iter != incompleteChildren.end();
         ++iter
         )
        deps.push_back(iter->first);
}

// add all of my virtual functions to 'vtb'
void BTypeDef::extendVTables(VTableBuilder &vtb) {
    // find all of the virtual functions
    for (Namespace::VarDefMap::iterator varIter = beginDefs();
         varIter != endDefs();
         ++varIter
         ) {

        // TODO: Try removing this loop, we shouldn't ever be storing FuncDefs.
        BFuncDef *funcDef = BFuncDefPtr::rcast(varIter->second);
        if (funcDef && (funcDef->flags & FuncDef::virtualized)) {
            vtb.add(funcDef);
            continue;
        }

        // check for an overload (if it's not an overload, assume that
        // it's not a function).  Iterate over all of the overloads at
        // the top level - the parent classes have
        // already had their shot at extendVTables, and we don't want
        // their overloads to clobber ours.
        OverloadDef *overload =
            OverloadDefPtr::rcast(varIter->second);
        if (overload)
            for (OverloadDef::FuncList::iterator fiter =
                  overload->beginTopFuncs();
                 fiter != overload->endTopFuncs();
                 ++fiter
                 )
                if ((*fiter)->flags & FuncDef::virtualized &&

                    // Make sure it's not an alias.  We don't want to add
                    // functions referenced by aliases to the vtable.
                    (*fiter)->name == overload->name
                    )
                    vtb.add(BFuncDefPtr::arcast(*fiter));
    }
}

/**
 * Create all of the vtables for 'type'.
 *
 * @param vtb the vtable builder
 * @param name the name stem for the VTable global variables.
 * @param vtableBaseType the global vtable base type.
 * @param firstVTable if true, we have not yet discovered the first
 *  vtable in the class schema.
 */
void BTypeDef::createAllVTables(VTableBuilder &vtb, const string &name,
                                bool firstVTable
                                ) {
    // if this is VTableBase, we need to create the VTable.
    // This is a special case: we should only get here when
    // initializing VTableBase's own vtable.
    if (this == vtb.vtableBaseType)
        vtb.createVTable(this, name, true);

    // iterate over the instance bases, construct VTables for all
    // ancestors that require them.
    TypeVec bases = getInstanceBases();

    for (TypeVec::iterator baseIter = bases.begin();
         baseIter != bases.end();
         ++baseIter
         ) {
        BTypeDef *base = BTypeDefPtr::arcast(*baseIter);

        // if the base class is VTableBase, we've hit bottom -
        // construct the initial vtable and store the first vtable
        // type if this is it.
        if (base == vtb.vtableBaseType) {
            vtb.createVTable(this, name, firstVTable);

            // otherwise, if the base has a vtable, create all of its
            // vtables
        } else if (base->hasVTable) {
            if (firstVTable)
                base->createAllVTables(vtb, name, firstVTable);
            else
                base->createAllVTables(vtb,
                                       name + ':' + base->getFullName(),
                                       firstVTable
                                       );
        }

        firstVTable = false;
    }

    // we must either have ancestors with vtables or be vtable base.
    assert(!firstVTable || this == vtb.vtableBaseType);

    // add my functions to their vtables
    extendVTables(vtb);
}

void BTypeDef::addPlaceholder(PlaceholderInstruction *inst) {
    assert(!complete && "Adding placeholder to a completed class");
    placeholders.push_back(inst);
}

BTypeDef *BTypeDef::findFirstVTable(BTypeDef *vtableBaseType) {

    // special case - if this is VTableBase, it is its own first vtable
    // (normally it is the first class to derive from VTableBase that is the
    // first vtable).
    if (this == vtableBaseType)
        return this;

    // check the parents
    for (TypeVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         ) {
        if (parent->get() == vtableBaseType) {
            return this;
        } else if ((*parent)->hasVTable) {
            BTypeDef *par = BTypeDefPtr::arcast(*parent);
            return par->findFirstVTable(vtableBaseType);
        }
    }

    SPUG_CHECK(false, "Failed to find first vtable for " << getFullName());
}

GlobalVariable *BTypeDef::getClassInstRep(Context &context,
                                          BModuleDef *module
                                          ) {
    if (classInstModuleId == module->repId) {
        return classInst;
    } else {
        GlobalVariable *gvar =
            cast_or_null<GlobalVariable>(
                module->rep->getGlobalVariable(getFullName() + ":body")
            );
        if (!gvar) {
            if (weak) {
                gvar = createClassImpl(context);
                createEmptyOffsetsInitializer(context);
            } else {
                // Make sure we aren't supposed to be the owner of this type.
                SPUG_CHECK(getModule() != module,
                           "Module " << module->getFullName() <<
                            " is missing the definiton for " << getFullName()
                           );
                gvar = new GlobalVariable(*module->rep,
                                          classInstType,
                                          true, // is constant
                                          GlobalValue::ExternalLinkage,
                                          0, // initializer: null for externs
                                          getFullName() + ":body"
                                          );
            }
        }

        classInst = gvar;
        classInstModuleId = module->repId;
        return gvar;
    }
}

GlobalVariable *BTypeDef::createClassInst(BModuleDef *module,
                                          StructType *metaClassStructType,
                                          Constant *classObjVal,
                                          const string &canonicalName
                                          ) {
    classInst = new GlobalVariable(*module->rep, metaClassStructType,
                                   true, // is constant
                                   weak ? GlobalValue::WeakAnyLinkage :
                                          GlobalValue::ExternalLinkage,
                                   classObjVal,
                                   canonicalName + ":body"
                                   );
    classInstType = metaClassStructType;
    classInstModuleId = module->repId;
    return classInst;
}

void BTypeDef::addDependent(BTypeDef *type, Context *context) {
    incompleteChildren.push_back(pair<BTypeDefPtr, ContextPtr>(type, context));
}

void BTypeDef::createEmptyOffsetsInitializer(Context &context) {
    LLVMBuilder *builder = LLVMBuilderPtr::cast(&context.builder);
    Module *module = builder->module;
    ArrayType *offsetsArrayType = ArrayType::get(builder->intzLLVM, 0);
    Constant *offsetsArrayInit =
        ConstantArray::get(offsetsArrayType, ArrayRef<Constant *>());
    string offsetsVarName = getFullName() +  ":offsets";
    GlobalVariable *offsetsVar =
        module->getGlobalVariable(offsetsVarName);
    if (offsetsVar)
        offsetsVar->setInitializer(offsetsArrayInit);
    else
        new GlobalVariable(*builder->module,
                           offsetsArrayType,
                           true, // is constant
                           GlobalValue::ExternalLinkage,
                           offsetsArrayInit,
                           offsetsVarName
                           );
}

Constant *BTypeDef::getParentOffset(const LLVMBuilder &builder, int parentIndex) const {
    // For appendages, all base classes are of offset 0.
    if (appendage)
        return Constant::getNullValue(builder.intzLLVM);

    // get the pointer to the inner "Class" object of "Class[BaseName]"
    BTypeDefPtr base = BTypeDefPtr::arcast(parents[parentIndex]);

    // calculate the offset from the beginning of the new classes
    // instance space to that of the base.
    Type *int32Type = Type::getInt32Ty(getGlobalContext());
    Constant *index0n[] = {
        ConstantInt::get(int32Type, 0),
        ConstantInt::get(int32Type, parentIndex)
    };
    return
        ConstantExpr::getPtrToInt(
            ConstantExpr::getGetElementPtr(
                Constant::getNullValue(cast<PointerType>(rep)),
                ArrayRef<Constant *>(index0n, 2)
            ),
            builder.intzLLVM
        );
}

void BTypeDef::createBaseOffsets(Context &context) const {
    // generate the offsets array (containing offsets to base classes)
    vector<Constant *> offsetsVal(parents.size());
    Type *int32Type = Type::getInt32Ty(getGlobalContext());
    LLVMBuilderPtr builder = LLVMBuilderPtr::cast(&context.builder);
    Type *intzType = builder->intzLLVM;
    for (int i = 0; i < parents.size(); ++i)
        offsetsVal[i] = getParentOffset(*builder, i);
    GlobalVariable *offsetsVar;
    ArrayType *offsetsArrayType =
        ArrayType::get(intzType, parents.size());

    string fullName = owner ?
        getFullName() :
        context.parent->ns->getNamespaceName() + "." + name;

    if (this == context.construct->classType.get())
        // .builtin.Class - we have to create the global variable.
        offsetsVar =
            new GlobalVariable(*builder->module,
                               offsetsArrayType,
                               true, // is constant
                               GlobalValue::ExternalLinkage,
                               NULL, // initializer, filled in later.
                               getFullName() +  ":offsets"
                               );
    else
        offsetsVar =
            builder->module->getGlobalVariable(fullName + ":offsets");
    SPUG_CHECK(offsetsVar,
               "Offsets variable not found for type " << fullName
               );
    Constant *offsetsArrayInit =
        ConstantArray::get(offsetsArrayType, offsetsVal);
    offsetsVar->setInitializer(offsetsArrayInit);
}

void BTypeDef::fixVTableSlots(int offset) {
    SPUG_FOR(VarDefMap, iter, defs) {
        OverloadDef *ovld = OverloadDefPtr::rcast(iter->second);
        if (ovld) {
            for (OverloadDef::FuncList::iterator fiter = ovld->beginTopFuncs();
                 fiter != ovld->endTopFuncs();
                 ++fiter
                 ) {
                if ((*fiter)->flags & FuncDef::virtualized &&
                    (*fiter)->vtableSlot >= firstVTableSlot
                    )
                    (*fiter)->vtableSlot += offset;
            }
        }
    }
}

void BTypeDef::fixIncompletes(Context &context) {
    // construct the vtable if necessary
    if (hasVTable) {
        if (parents.size()) {
            int lastBaseVTableSlot =
                BTypeDefPtr::rcast(parents[0])->nextVTableSlot;

            // There can't be a gap between the last base class vtable slot and
            // the derived class slot.
            SPUG_CHECK(lastBaseVTableSlot >= firstVTableSlot,
                       "First vtable slot of " << name << " is " <<
                        firstVTableSlot << " which is greater than the last "
                        "vtable slot of base class (" << lastBaseVTableSlot << ")"
                       );

            if (firstVTableSlot < lastBaseVTableSlot)
                fixVTableSlots(lastBaseVTableSlot - firstVTableSlot);
        }

        VTableBuilder vtableBuilder(
            dynamic_cast<LLVMBuilder*>(&context.builder),
            BTypeDefPtr::arcast(context.construct->vtableBaseType)
        );
        createAllVTables(
            vtableBuilder,
            ".vtable." + context.parent->ns->getNamespaceName() + "." + name
        );
        vtableBuilder.emit(this);
    }

    createBaseOffsets(context);

    // fix-up all of the placeholder instructions
    for (vector<PlaceholderInstruction *>::iterator iter =
            placeholders.begin();
         iter != placeholders.end();
         ++iter
         )
        (*iter)->fix();
    placeholders.clear();

    // fix up all incomplete children
    for (IncompleteChildVec::iterator iter = incompleteChildren.begin();
         iter != incompleteChildren.end();
         ++iter
         )
        iter->first->fixIncompletes(*iter->second);
    incompleteChildren.clear();

    complete = true;
}

void BTypeDef::materializeVTable(Context &context) {
    if (hasVTable) {
        VTableBuilder vtb(LLVMBuilderPtr::cast(&context.builder),
                          BTypeDefPtr::arcast(context.construct->vtableBaseType)
                          );
        createAllVTables(vtb, ".vtable." + getFullName());
        vtb.materialize(this);
    }
}

void BTypeDef::setClassInst(llvm::GlobalVariable *classInst) {
    SPUG_CHECK(!this->classInst && !classInstType,
               "Resetting class instance for " << getDisplayName()
               );
    this->classInst = classInst;
    this->classInstType = classInst->getType()->getElementType();
}

int BTypeDef::countAncestors() const {
    int result = 0;
    SPUG_FOR(TypeVec, iter, parents) {
        ++result;
        result += BTypeDefPtr::arcast(*iter)->countAncestors();
    }
    return result;
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

GlobalVariable *BTypeDef::createClassImpl(Context &context) {

    LLVMContext &lctx = getGlobalContext();
    LLVMBuilder &llvmBuilder = dynamic_cast<LLVMBuilder &>(context.builder);

    GlobalValue::LinkageTypes linkage =
        weak ? GlobalValue::WeakAnyLinkage : GlobalValue::ExternalLinkage;

    // in situations where we haven't defined the class body yet, create a
    // fake class containing the parents so we can compute the base class
    // offsets.
    PointerType *pointerType = rep ? dyn_cast<PointerType>(rep) : 0;
    StructType *instType =
        pointerType ? dyn_cast<StructType>(pointerType->getElementType()) : 0;
    if (!instType || instType->isOpaque()) {
        instType = StructType::create(lctx);
        vector<Type *> fields(parents.size());
        for (int i = 0; i < parents.size(); ++i) {
            PointerType *basePtrType =
                cast<PointerType>(
                    BTypeDefPtr::rcast(parents[i])->rep
                );
            fields[i] = basePtrType->getElementType();
        }
        instType->setBody(fields);
        pointerType = instType->getPointerTo();
    }

    Type *intzType =
        BTypeDefPtr::arcast(context.construct->intzType)->rep,

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
    // if the type has an owner, getFullName() will do this, but at
    // this stage the type itself usually doesn't have an owner yet, so we
    // build it from context.
    string canonicalName(getOwner() ? getFullName() :
                                      earlyCanonicalize(context, name)
                         );

    // name
    Constant *nameInit = ConstantDataArray::getString(lctx, name, true);
    GlobalVariable *nameGVar =
        new GlobalVariable(*llvmBuilder.module,
                           nameInit->getType(),
                           true, // is constant
                           linkage,
                           nameInit,
                           canonicalName + ":name"
                           );
    classStructVals[0] =
        ConstantExpr::getGetElementPtr(nameGVar, index00, 2);

    // numBases
    Type *uintType = BTypeDefPtr::arcast(context.construct->uintType)->rep;
    classStructVals[1] = ConstantInt::get(uintType, parents.size());

    // bases
    vector<Constant *> basesVal(parents.size());
    for (int i = 0; i < parents.size(); ++i) {
        // get the pointer to the inner "Class" object of "Class[BaseName]"
        BTypeDefPtr base = BTypeDefPtr::arcast(parents[i]);

        // get the class body global variable
        Constant *baseClassPtr =
            base->getClassInstRep(context, llvmBuilder.moduleDef.get());

        // 1) if the base class is Class, just use the pointer
        // 2) if it's Class[BaseName], GEP our way into the base class
        // (Class) instance.
        if (base->type.get() == base.get())
            basesVal[i] = baseClassPtr;
        else
            basesVal[i] =
                ConstantExpr::getGetElementPtr(baseClassPtr, index00, 2);

        SPUG_CHECK(basesVal[i]->getType() == classType->rep,
                   "Base " << i << " of class " << getFullName() <<
                    " has an LLVM type that is not that of Class"
                   );
    }
    ArrayType *baseArrayType =
        ArrayType::get(classType->rep, parents.size());
    Constant *baseArrayInit = ConstantArray::get(baseArrayType, basesVal);
    GlobalVariable *basesGVar =
        new GlobalVariable(*llvmBuilder.module,
                           baseArrayType,
                           true, // is constant
                           linkage,
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
                               ArrayType::get(intzType, parents.size()),
                               true, // is constant
                               linkage,
                               NULL, // initializer, filled in later.
                               canonicalName +  ":offsets"
                               );
    classStructVals[3] =
        ConstantExpr::getGetElementPtr(offsetsGVar, index00, 2);

    // Create numVTables.
    classStructVals[4] =
        ConstantInt::get(uintType, countRootAncestors() * 2);

    // Create the vtables array.  This is an array of vtable address, offset
    // from the beginning of the instances.
    Type *intPtrType =
        Type::getInt8Ty(getGlobalContext())->getPointerTo();
    int rootCount;
    if (this == context.construct->vtableBaseType.get())
        rootCount = 1;
    else if (appendage) {
        rootCount = getAnchorType()->countRootAncestors();
    } else {
        rootCount = countRootAncestors();
    }
    Type *voidptrArrayType = ArrayType::get(intPtrType, rootCount * 2);
    if (hasVTable) {
        GlobalVariable *vtablesGVar =
            new GlobalVariable(*llvmBuilder.module,
                               voidptrArrayType,
                               true, // is constant
                               linkage,
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
        cast<PointerType>(BTypeDefPtr::arcast(type)->rep);
    StructType *metaClassStructType =
        cast<StructType>(metaClassPtrType->getElementType());

    // initialize the structure based on whether we're implementing an
    // instance of Class or an instance of a meta-class derived from Class.
    Constant *classObjVal;
    if (type.get() != context.construct->classType.get()) {

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
        createClassInst(llvmBuilder.moduleDef.get(),
                        metaClassStructType,
                        classObjVal,
                        canonicalName
                        );

    // Get or create the pointer to the class instance.  We have to do the
    // "get" first because, in the case of OverloadTypes (and possibly for
    // other builtin generics), the type object can get created as an external
    // (and merged into the merged module) before it is created during
    // deserialization of meta-data.
    GlobalVariable *classInstPtr =
        llvmBuilder.module->getGlobalVariable(canonicalName);
    if (!classInstPtr)
        classInstPtr =
            new GlobalVariable(*llvmBuilder.module, metaClassPtrType,
                               true, // is constant
                               linkage,
                               classInst,
                               canonicalName
                               );
    else if (!classInstPtr->hasInitializer())
        // If we already had it, set the initializer.
        classInstPtr->setInitializer(classInst);

    // store the impl object in the class
    impl = weak ? new BWeakClassVarDefImpl(classInstPtr,
                                           llvmBuilder.moduleDef->repId,
                                           this
                                           ) :
                  new BGlobalVarDefImpl(classInstPtr,
                                        llvmBuilder.moduleDef->repId
                                        );
    return classInstPtr;
}
