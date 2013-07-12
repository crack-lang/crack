// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "VTableBuilder.h"

#include "BTypeDef.h"
#include "BFuncDef.h"
#include "model/Context.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/GlobalVariable.h>
#include <vector>


using namespace std;
using namespace llvm;
using namespace model;
using namespace builder::mvll;

namespace {
    // utility function to resize a vector to accomodate a new element, but
    // only if necessary.
    template<typename T>
    void accomodate(vector<T *> &vec, size_t index) {
        if (vec.size() < index + 1)
            vec.resize(index + 1, 0);
    }
}

void VTableInfo::dump() {
    std::cerr << name << ":\n";
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i])
            entries[i]->dump();
        else
            std::cerr << "null entry!" << std::endl;
    }
}

void VTableBuilder::dump() {
    for (VTableMap::iterator iter = vtables.begin();
    iter != vtables.end();
    ++iter)
        iter->second->dump();
}

void VTableBuilder::addToAncestor(BTypeDef *ancestor, BFuncDef *func) {
    // lookup the vtable
    VTableMap::iterator iter = vtables.find(ancestor);

    // if we didn't find a vtable in the ancestors, append the
    // function to the first vtable (I don't think this can ever happen)
    VTableInfo *targetVTable;
    if (iter == vtables.end()) {
        assert(firstVTable && "no first vtable");
        targetVTable = firstVTable;
    } else {
        targetVTable = iter->second.get();
    }

    // insert the function
    vector<Constant *> &entries = targetVTable->entries;
    accomodate(entries, func->vtableSlot);
    Constant *funcRep = func->getRep(*builder);
    entries[func->vtableSlot] = funcRep;
}

// add the function to all vtables.
void VTableBuilder::addToAll(BFuncDef *func) {
    for (VTableMap::iterator iter = vtables.begin();
         iter != vtables.end();
         ++iter
         ) {
        vector<Constant *> &entries = iter->second->entries;
        accomodate(entries, func->vtableSlot);
        Constant *funcRep = func->getRep(*builder);
        entries[func->vtableSlot] = funcRep;
    }
}

// add a new function entry to the appropriate VTable
void VTableBuilder::add(BFuncDef *func) {

    // find the ancestor of "ancestor" with the first vtable
    BTypeDefPtr curType = func->receiverType;
    BTypeDef *ancestor = curType->findFirstVTable(vtableBaseType);

    // if the function comes from VTableBase, we have to insert
    // the function into _all_ of the vtables - this is because
    // all of them are derived from vtable base.  (the only such
    // function is "oper class")
    if (ancestor == vtableBaseType)
        addToAll(func);
    else
        addToAncestor(ancestor, func);

}

// create a new VTable
void VTableBuilder::createVTable(BTypeDef *type, const std::string &name,
                  bool first
                  ) {
    assert(vtables.find(type) == vtables.end());
    VTableInfo *info;
    vtables[type] = info = new VTableInfo(name);
    if (first)
        firstVTable = info;
}

void VTableBuilder::emit(BTypeDef *type) {
    for (VTableMap::iterator iter = vtables.begin();
         iter != vtables.end();
         ++iter
         ) {
        // populate the types array
        vector<Constant *> &entries = iter->second->entries;

        vector<Type *> vtableTypes(entries.size());
        int i = 0;
        for (vector<Constant *>::iterator entryIter =
             entries.begin();
             entryIter != entries.end();
             ++entryIter, ++i
             ) {
            assert(*entryIter && "Null vtable entry.");
            vtableTypes[i] = (*entryIter)->getType();
        }

        // create a constant structure that actually is the vtable
        
        // get the structure type from our global registry.
        StructType *vtableStructType =
            LLVMBuilder::getLLVMType(iter->second->name);
        if (!vtableStructType) {
            vtableStructType =
                StructType::create(getGlobalContext(), vtableTypes,
                                   iter->second->name
                                   );
            LLVMBuilder::putLLVMType(iter->second->name, vtableStructType);
        } else {
            // sanity check the type we retrieved.
            SPUG_CHECK(vtableTypes.size() == 
                        vtableStructType->getNumElements(),
                       "vtable type " << iter->second->name << 
                        " has a differently sized body.  Want " <<
                        vtableTypes.size() << ", got " <<
                        vtableStructType->getNumElements()
                       );
        }

        type->vtables[iter->first] =
                new GlobalVariable(*module, vtableStructType,
                                   true, // isConstant
                                   GlobalValue::ExternalLinkage,

                                   // initializer - this needs to be
                                   // provided or the global will be
                                   // treated as an extern.
                                   ConstantStruct::get(
                                       vtableStructType,
                                       iter->second->entries
                                   ),
                                   iter->second->name
                                   );

        // store the first VTable pointer (a pointer-to-pointer to the
        // struct type, actually, because that is what we need to cast our
        // VTableBase instances to)
        if (iter->second == firstVTable)
            type->firstVTableType =
                PointerType::getUnqual(
                    PointerType::getUnqual(vtableStructType)
                );
    }

    assert(type->firstVTableType);
}

void VTableBuilder::materialize(BTypeDef *type) {
    for (VTableMap::iterator iter = vtables.begin();
         iter != vtables.end();
         ++iter
         ) {

        Constant *gvar = module->getGlobalVariable(iter->second->name);
        SPUG_CHECK(gvar, 
                   "No global variable defined for vtable " << 
                    iter->second->name
                   );
        type->vtables[iter->first] = gvar;

        // store the first VTable pointer (a pointer-to-pointer to the
        // struct type, actually, because that is what we need to cast our
        // VTableBase instances to)
        if (iter->second == firstVTable) {
            type->firstVTableType = PointerType::getUnqual(gvar->getType());
            PointerType *pt = cast<PointerType>(gvar->getType());
            StructType *st = cast<StructType>(pt->getElementType());
            
            // next vtable slot is the size of the table
            type->nextVTableSlot = st->getNumElements();
        }
    }

    assert(type->firstVTableType);
}
