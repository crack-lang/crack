// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "StructResolver.h"

#include <assert.h>
#include <ctype.h>

#include <vector>
#include <iostream>

#include <llvm/ADT/StringMap.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/User.h>
#include <llvm/IR/Constants.h>
#include <llvm/Analysis/FindUsedTypes.h>

#include "spug/check.h"
#include "spug/stlutil.h"
#include "spug/StringFmt.h"
#include "spug/Tracer.h"
#include "LLVMBuilder.h"

using namespace llvm;
using namespace std;
using namespace builder::mvll;

#define SR_DEBUG if (StructResolver::trace)
bool StructResolver::trace = false;

namespace {

    spug::Tracer tracer("StructResolver", StructResolver::trace,
                        "LLVM module struct type normalization."
                        );

    const char *tName(int t) {
        if (t == 11)
            return "Structure";
        else if (t == 12)
            return "Array";
        else if (t == 13)
            return "Pointer";
        else
            return "-Other-";
    }

    template <typename T>
    string llvmStr(const T &obj) {
        string temp;
        raw_string_ostream out(temp);
        obj.print(out);
        return out.str();
    }

    class MapGuard {
        typedef map<Type *, bool> MapType;
        MapType &myMap;
        Type *type;
    public:
        MapGuard(MapType &myMap, Type *type) : myMap(myMap), type(type) {
            myMap[type] = true;
        }
        ~MapGuard() {
            MapType::iterator iter = myMap.find(type);
            SPUG_CHECK(iter != myMap.end(), "MapGuard RAII failed.");
            myMap.erase(iter);
        }
    };
} // anon namespace

Type *StructResolver::mapStructTypeByElements(StructType *type) {
    // See if the type already got mapped when doing the fields.
    StructResolver::StructMapType::iterator iter =
        typeMap.find(type);
    if (iter != typeMap.end())
        return iter->second;

    // Make the current type relinquish its name.
    string origName = type->getName();
    SPUG_CHECK(origName.substr(0, 3) != ":::",
               "re-renaming type " << origName
               );
    type->setName(SPUG_FSTR(":::" << origName << "." << type));

    SR_DEBUG cerr << "Rebuilding struct type " << origName <<
        " (depends on mapped types, old is now " <<
        string(type->getName()) << ")" << endl;

    SPUG_CHECK(!LLVMBuilder::getLLVMType(origName),
               "Renamed registered type " << origName
               );

    // Create a new type and add it to the map so that when we map the
    // elements, we'll fix recursive references to the new type.
    StructType *newType =
        StructType::create(type->getContext(), origName);
    typeMap[type] = newType;

    // Map all elements.
    vector<Type *> elements;
    for (StructType::element_iterator iter = type->element_begin();
         iter != type->element_end();
         ++iter
         ) {
        Type *elemType = *iter;
        if (Type *newType = mapType(*iter))
            elemType = newType;
        elements.push_back(elemType);
    }

    newType->setBody(elements, type->isPacked());
    LLVMBuilder::putLLVMType(origName, newType);

    return newType;
}

// 'type' should not be a duplicate.
Type *StructResolver::maybeMapStructTypeByElements(StructType *type) {

    // See if any of the element types must be mapped.
    bool mustMap = false;
    for (StructType::element_iterator iter = type->element_begin();
        iter != type->element_end();
        ++iter
        ) {
        if (mapType(*iter)) {
            mustMap = true;
            break;
        }
    }

    if (mustMap) {
        return mapStructTypeByElements(type);
    } else {
        // If we get here, this is the original instance of the type,
        // loaded from the cache, and it contains no nested types that
        // need to be converted.  Register it.
        typeMap[type] = type;
        LLVMBuilder::putLLVMType(string(type->getName()), type);

        return 0;
    }
}

Type *StructResolver::mapStructType(StructType *structTy) {
    // does it have a name?
    if (!structTy->hasName())
        return 0;

    if (spug::contains(traversed, structTy))
        return 0;
    MapGuard guard(traversed, structTy);

    string name = structTy->getName().str();

    // does it have a numeric suffix? (i.e. is it a duplicate type name?)
    int pos = name.rfind(".");
    bool isDuplicate = false;
    if (pos != string::npos && name.size() > pos) {
        string suffix = name.substr(pos + 1);
        for (string::const_iterator si = suffix.begin();
            si != suffix.end();
            ++si
            ) {
            isDuplicate = isdigit(*si);
            if (!isDuplicate)
                break;
        }
    }

    if (!isDuplicate) {
        // The type itself isn't a duplicate.  Now we have to check all
        // nested types since these might have been duplicates even if the
        // outer type wasn't.
        return maybeMapStructTypeByElements(structTy);
    }

    string canonicalName = name.substr(0, pos);

    SR_DEBUG cerr << "Mapping type " << structTy->getName().str() <<
        " to " << canonicalName << endl;

    // The numeric suffix is only applied when we discover a new type
    // with the same name.  Therefore, the fact that there is a
    // numeric suffix implies that we should have already loaded the
    // type.
    StructType *type = LLVMBuilder::getLLVMType(canonicalName);

    if (!type) {
        // We seem to be able to get into a situation where we get a duplicate
        // type that hasn't been registered yet, I'm not exactly sure why but
        // I've observed it with cyclic modules.  Try changing the name and
        // remapping the type.
        SR_DEBUG cerr << "Unregistered duplicate type found for " <<
            canonicalName << endl;
        structTy->setName(canonicalName);
        SPUG_CHECK(structTy->getName() == canonicalName,
                   "Failed to fix name of unregistered duplicate type " <<
                   canonicalName);
        return mapStructTypeByElements(structTy);
    } else if (type->getName() != canonicalName) {
        // This was to help me debug something.
        SR_DEBUG cerr << "Restoring missing name for original type " <<
            canonicalName << " (was " << llvmStr(*type) << ")" << endl;
        type->setName(canonicalName);
        if (type == structTy)
            return 0;
    }

    StructType *left = structTy;
    assert(left);
    StructType *right = type;
    SPUG_CHECK(left != right,
               "Mapping type " << canonicalName << " to itself");
    //cout << "struct map [" << left->getName().str() << "] to [" << right->getName().str() << "]\n";
    typeMap[left] = right;
    return right;
}

void StructResolver::traceMapping(Type *type, Type *newType,
                                  const char *designator
                                  ) {
    cerr << "Mapping " << designator << " type " << llvmStr(*type) <<
        " to " << llvmStr(*newType) << endl;
}

Type *StructResolver::mapArrayType(ArrayType *type) {
    Type *mappedElemType = mapType(type->getElementType());
    if (mappedElemType) {
        Type *newType =
            ArrayType::get(mappedElemType, type->getNumElements());
        if (StructResolver::trace)
            traceMapping(type, newType, "array");
        typeMap[type] = newType;
        return newType;
    } else {
        return 0;
    }
}

Type *StructResolver::mapPointerType(PointerType *type) {
    Type *mappedElemType = mapType(type->getElementType());
    if (mappedElemType) {
        Type *newType =
            PointerType::get(mappedElemType, type->getAddressSpace());
        if (StructResolver::trace)
            traceMapping(type, newType, "pointer");
        typeMap[type] = newType;
        return newType;
    } else {
        return 0;
    }
}

Type *StructResolver::mapFunctionType(FunctionType *type) {
    bool needsMap = false;
    Type *returnType = type->getReturnType();
    Type *newType;
    if (newType = mapType(returnType)) {
        needsMap = true;
        returnType = newType;
    }

    vector<Type *> parmTypes;
    for (FunctionType::param_iterator iter = type->param_begin();
        iter != type->param_end();
        ++iter
        ) {
        Type *paramType = *iter;
        if (newType = mapType(paramType)) {
            needsMap = true;
            paramType = newType;
        }
        parmTypes.push_back(paramType);
    }

    if (needsMap) {
        newType = FunctionType::get(returnType, parmTypes,
                                    type->isVarArg()
                                    );
        if (StructResolver::trace)
            traceMapping(type, newType, "function");
        typeMap[type] = newType;
        return newType;
    } else {
        return 0;
    }
}

Type *StructResolver::mapType(Type *type) {
    StructResolver::StructMapType::iterator iter = typeMap.find(type);
    if (iter != typeMap.end())
        return iter->second;

    switch (type->getTypeID()) {
        case Type::StructTyID:
            return mapStructType(cast<StructType>(type));
        case Type::ArrayTyID:
            return mapArrayType(cast<ArrayType>(type));
        case Type::PointerTyID:
            return mapPointerType(cast<PointerType>(type));
        case Type::FunctionTyID:
            return mapFunctionType(cast<FunctionType>(type));
    }
    return 0;
}

void StructResolver::buildTypeMap() {

    SetVector<Type *> usedTypes;

    FindUsedTypes findTypes;
    findTypes.runOnModule(*module);
    usedTypes = findTypes.getTypes();

    // NOTE this gets a list by struct name only: no isomorphism checks

    for (SetVector<Type *>::iterator i = usedTypes.begin();
         i != usedTypes.end(); ++i
         ) {
        // See if we've already mapped the type.
        if (spug::contains(typeMap, *i))
            continue;

        mapType(*i);
    }
}

Type *StructResolver::remapType(Type *srcType) {
    StructMapType::const_iterator iter = typeMap.find(srcType);
    if (iter != typeMap.end())
        return iter->second;
    else
        return srcType;
}
