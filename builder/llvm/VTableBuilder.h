// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_VTableBuilder_h_
#define _builder_llvm_VTableBuilder_h_

#include <spug/RCPtr.h>
#include <spug/RCBase.h>

#include <map>
#include <string>
#include <vector>

namespace llvm {
    class Constant;
    class Module;
}

namespace builder {
namespace mvll {

class BTypeDef;
class BFuncDef;

SPUG_RCPTR(VTableInfo);

// stores information on a single VTable
class VTableInfo : public spug::RCBase {
private:
    // give an error if we try to copy this
    VTableInfo(const VTableInfo &other);

public:
    // the vtable variable name
    std::string name;

    // the vtable entries
    std::vector<llvm::Constant *> entries;

    VTableInfo(const std::string &name) : name(name) {}

    void dump();

};

// encapsulates all of the vtables for a new type
class VTableBuilder {
private:
    typedef std::map<BTypeDef *, VTableInfoPtr> VTableMap;
    VTableMap vtables;

    // keep track of the first VTable in the type
    VTableInfo *firstVTable;

    BTypeDef *vtableBaseType;

    // used for emmission, but also IR type naming
    llvm::Module *module;

public:
    VTableBuilder(BTypeDef *vtableBaseType, llvm::Module *m) :
            firstVTable(0),
            vtableBaseType(vtableBaseType),
            module(m) {
    }

    void dump();

    void addToAncestor(BTypeDef *ancestor, BFuncDef *func);

    // add the function to all vtables.
    void addToAll(BFuncDef *func);

    // add a new function entry to the appropriate VTable
    void add(BFuncDef *func);

    // create a new VTable
    void createVTable(BTypeDef *type, const std::string &name,
                      bool first);

    // emit all of the VTable globals
    void emit(BTypeDef *type);

};

} // end namespace builder::vmll
} // end namespace builder

#endif
