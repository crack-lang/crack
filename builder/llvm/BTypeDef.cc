// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "BFuncDef.h"
#include "BTypeDef.h"

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>
#include "model/Context.h"
#include "model/OverloadDef.h"
#include "PlaceholderInstruction.h"
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

    // iterate over the base classes, construct VTables for all
    // ancestors that require them.
    for (TypeVec::iterator baseIter = parents.begin();
         baseIter != parents.end();
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

void BTypeDef::addBaseClass(BTypeDef *base) {
    ++fieldCount;
    parents.push_back(base);
    if (base->hasVTable)
        hasVTable = true;
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

GlobalVariable *BTypeDef::getClassInstRep(BModuleDef *module) {
    if (classInstModuleId == module->repId) {
        return classInst;
    } else {
        GlobalVariable *gvar = 
            cast_or_null<GlobalVariable>(
                module->rep->getGlobalVariable(getFullName() + ":body")
            );
        if (!gvar) {
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

void BTypeDef::createBaseOffsets(Context &context) const {
    // generate the offsets array (containing offsets to base classes)
    PointerType *pointerType = cast<PointerType>(rep);
    vector<Constant *> offsetsVal(parents.size());
    Type *int32Type = Type::getInt32Ty(getGlobalContext());
    Constant *zero = ConstantInt::get(int32Type, 0);
    LLVMBuilderPtr builder = LLVMBuilderPtr::cast(&context.builder);
    Type *intzType = builder->intzLLVM;
    for (int i = 0; i < parents.size(); ++i) {
        // get the pointer to the inner "Class" object of "Class[BaseName]"
        BTypeDefPtr base = BTypeDefPtr::arcast(parents[i]);

        // calculate the offset from the beginning of the new classes 
        // instance space to that of the base.
        Constant *index0n[] = {
            zero,
            ConstantInt::get(int32Type, i)
        };
        offsetsVal[i] =
            ConstantExpr::getPtrToInt(
                ConstantExpr::getGetElementPtr(
                    Constant::getNullValue(pointerType),
                    ArrayRef<Constant *>(index0n, 2)
                ),
                intzType
            );
    }
    GlobalVariable *offsetsVar;
    ArrayType *offsetsArrayType = 
        ArrayType::get(intzType, parents.size());
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
            builder->module->getGlobalVariable(getFullName() + ":offsets");
    SPUG_CHECK(offsetsVar, 
               "Offsets variable not found for type " << getFullName()
               );
    Constant *offsetsArrayInit = 
        ConstantArray::get(offsetsArrayType, offsetsVal);
    offsetsVar->setInitializer(offsetsArrayInit);
}

void BTypeDef::fixIncompletes(Context &context) {
    // construct the vtable if necessary
    if (hasVTable) {
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
