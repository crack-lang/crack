// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "VTableBuilder.h"

#include "BTypeDef.h"
#include "BFuncDef.h"
#include "model/Context.h"

#include <llvm/LLVMContext.h>
#include <llvm/GlobalVariable.h>
#include <vector>


using namespace std;
using namespace llvm;
using namespace model;

namespace {
    // utility function to resize a vector to accomodate a new element, but
    // only if necessary.
    template<typename T>
    void accomodate(vector<T *> &vec, size_t index) {
        if (vec.size() < index + 1)
            vec.resize(index + 1, 0);
    }
}

namespace builder {
namespace mvll {

void VTableInfo::dump() {
    std::cerr << name << ":\n";
    for (int i = 0; i < entries.size(); ++i) {
        if (entries[i])
            entries[i]->dump();
        else
            std::cerr << "null entry" << std::endl;
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
    // function to the first vtable
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
    entries[func->vtableSlot] = (Constant*)func->rep;
}

// add the function to all vtables.
void VTableBuilder::addToAll(BFuncDef *func) {
    for (VTableMap::iterator iter = vtables.begin();
    iter != vtables.end();
    ++iter
            ) {
        vector<Constant *> &entries = iter->second->entries;
        accomodate(entries, func->vtableSlot);
        entries[func->vtableSlot] = (Constant*)func->rep;
    }
}

// add a new function entry to the appropriate VTable
void VTableBuilder::add(BFuncDef *func) {

    // find the ancestor whose vtable this function needs to go
    // into
    BTypeDef *ancestor;
    TypeDef::AncestorPath &path = func->pathToFirstDeclaration;
    if (path.size())
        ancestor = BTypeDefPtr::arcast(path.back().ancestor);
    else
        ancestor = BTypeDefPtr::acast(func->owner);

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

void VTableBuilder::emit(Module *module, BTypeDef *type) {
    for (VTableMap::iterator iter = vtables.begin();
    iter != vtables.end();
    ++iter
            ) {
        // populate the types array
        vector<Constant *> &entries = iter->second->entries;
        vector<const Type *> vtableTypes(entries.size());
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
        const StructType *vtableStructType =
                StructType::get(getGlobalContext(), vtableTypes);
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

} // end namespace builder::vmll
} // end namespace builder

