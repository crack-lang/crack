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
    StructMapType *typeMap;
    StructMapType reverseMap;
    std::map<llvm::Value *, bool> visited;
    std::map<llvm::Type *, int> visitedStructs;

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
    StructResolver(llvm::Module *mod0): module(mod0), typeMap(0) { }

    // return the list of structs which conflict (by name only) with existing
    // definitions in the current llvm context. these are recognized by
    // their numeric suffix, e.g. foo.0, bar.1, etc.
    StructListType getDisjointStructs();

    void run(StructMapType *m);

};

}} // namespace builder::mvll

#endif
