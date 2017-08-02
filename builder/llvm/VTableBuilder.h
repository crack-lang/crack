// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_VTableBuilder_h_
#define _builder_llvm_VTableBuilder_h_

#include <spug/RCPtr.h>
#include <spug/RCBase.h>

#include <map>
#include <string>
#include <vector>

#include "BTypeDef.h"
#include "LLVMBuilder.h"  // need this because we are accessing its methods

namespace llvm {
    class Constant;
    class Module;
}

namespace builder {
namespace mvll {

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

    // used for emmission, but also IR type naming
    llvm::Module *module;

    // used to get correct Function* for the builder's Module*
    LLVMBuilder *builder;

    // Fills the initializer array for the class' vtables variable.
    int fillVTablesVar(std::vector<llvm::Constant *> &vtablesArrayInit,
                        int outputStart,
                        const BTypeDef::VTableMap &vtabMap,
                        BTypeDef *type,
                        int start,
                        llvm::Constant *initOffset
                        );
public:
    BTypeDef *vtableBaseType;

    VTableBuilder(LLVMBuilder *b,
                  BTypeDef *vtableBaseType) :
            firstVTable(0),
            vtableBaseType(vtableBaseType),
            module(b->module),
            builder(b) {
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

    /**
     * Materialize the vtable structures in the type by extracting them from
     * the module.
     */
    void materialize(BTypeDef *type);

};

} // end namespace builder::mvll
} // end namespace builder

#endif
