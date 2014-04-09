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

namespace llvm {
    class Module;
    class StructType;
    class Type;
    class User;
    class Value;
    class Function;
}

namespace builder {
namespace mvll {

class StructResolver {

public:
    typedef std::map<llvm::Type*, llvm::Type*> StructMapType;
    typedef std::map<std::string, llvm::Type*> StructListType;
    static bool trace;

protected:
    llvm::Module *module;
    StructMapType typeMap;
    StructMapType reverseMap;
    std::map<llvm::Value *, bool> visited;
    std::map<llvm::Type *, int> visitedStructs;
    typedef std::map<llvm::Value *, llvm::Type *> ValueTypeMap;
    ValueTypeMap originalTypes;

    llvm::Type *maybeGetMappedType(llvm::Type *t);
    void mapValue(llvm::Value &val);
    void mapUser(llvm::User &val);
    void mapFunction(llvm::Function &fun);

    void mapGlobals();
    void mapFunctions();
    void mapMetadata();
    bool buildElementVec(llvm::StructType *type,
                         std::vector<llvm::Type *> &elems
                         );

public:
    StructResolver(llvm::Module *mod0): module(mod0) {}

    // Build the type map for the module - this is the mapping from the
    // duplicated struct types (with numeric suffixes in their names) to
    // the actual types.
    StructListType buildTypeMap();

    void run();

    // Restores the original types of constants.  This is necessary because
    // LLVM looks up the constant based on its type during destruction.
    void restoreOriginalTypes();
};

}} // namespace builder::mvll

#endif
