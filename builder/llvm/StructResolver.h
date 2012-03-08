// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>

#ifndef StructRewrite_h_
#define StructRewrite_h_

#include <map>
#include <string>

namespace llvm {
    class Module;
    class Type;
    class User;
    class Value;
}

class StructResolver {

public:
    typedef std::map<llvm::Type*, llvm::Type*> StructMapType;
    typedef std::map<std::string, llvm::Type*> StructListType;

protected:
    llvm::Module *module;
    StructMapType *typeMap;
    std::map<llvm::Value*, bool> visited;

    llvm::Type *maybeGetMappedType(llvm::Type *t);
    void mapValue(llvm::Value &val);
    void mapUser(llvm::User &val);

    void mapGlobals();
    void mapFunctions();


public:
    StructResolver(llvm::Module *mod0): module(mod0), typeMap(0) { }

    // return the list of structs which conflict (by name only) with existing
    // definitions in the current llvm context. these are recognized by
    // their numeric suffix, e.g. foo.0, bar.1, etc.
    StructListType getDisjointStructs();

    void run(StructMapType *m);

};

#endif
