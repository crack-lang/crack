// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef StructRewrite_h_
#define StructRewrite_h_

#include <map>
#include <string>
#include <vector>

#include <llvm/Transforms/Utils/ValueMapper.h>

namespace llvm {
    class ArrayType;
    class FunctionType;
    class Module;
    class PointerType;
    class StructType;
    class Type;
    class User;
    class Value;
    class Function;
}

namespace builder {
namespace mvll {

class StructResolver : public llvm::ValueMapTypeRemapper {

public:
    typedef std::map<llvm::Type*, llvm::Type*> StructMapType;
    static bool trace;

protected:
    llvm::Module *module;
    StructMapType typeMap;
    std::map<llvm::Type *, bool> traversed;

    llvm::Type *mapStructTypeByElements(llvm::StructType *type);
    llvm::Type *mapStructType(llvm::StructType *structTy);
    void traceMapping(llvm::Type *type, llvm::Type *newType,
                      const char *designator
                      );
    llvm::Type *mapArrayType(llvm::ArrayType *type);
    llvm::Type *mapPointerType(llvm::PointerType *type);
    llvm::Type *mapFunctionType(llvm::FunctionType *type);
    llvm::Type *mapType(llvm::Type *type);

public:
    StructResolver(llvm::Module *mod0): module(mod0) {}

    // Build the type map for the module - this is the mapping from the
    // duplicated struct types (with numeric suffixes in their names) to
    // the actual types.
    void buildTypeMap();

    // Implements ValueMapTypeRemapper interface.
    virtual llvm::Type *remapType(llvm::Type *srcType);
};

}} // namespace builder::mvll

#endif
