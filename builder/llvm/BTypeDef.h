// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BTypeDef_h_
#define _builder_llvm_BTypeDef_h_

#include "model/TypeDef.h"
#include "VTableBuilder.h"

#include <map>
#include <string>

namespace llvm {
    class Type;
    class Constant;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BTypeDef);

// (RCPTR defined above)
class BTypeDef : public model::TypeDef {
public:
    const llvm::Type *rep;
    unsigned nextVTableSlot;

    // mapping from base types to their vtables.
    std::map<BTypeDef *, llvm::Constant *> vtables;
    const llvm::Type *firstVTableType;

    BTypeDef(TypeDef *metaType, const std::string &name,
             const llvm::Type *rep,
             bool pointer = false,
             unsigned nextVTableSlot = 0
                                       ) :
    model::TypeDef(metaType, name, pointer),
    rep(rep),
    nextVTableSlot(nextVTableSlot),
    firstVTableType(0) {
    }

    // add all of my virtual functions to 'vtb'
    void extendVTables(VTableBuilder &vtb);

    /**
         * Create all of the vtables for 'type'.
         * 
         * @param vtb the vtable builder
         * @param name the name stem for the VTable global variables.
         * @param vtableBaseType the global vtable base type.
         * @param firstVTable if true, we have not yet discovered the first 
         *  vtable in the class schema.
         */
    void createAllVTables(VTableBuilder &vtb, const std::string &name,
                          BTypeDef *vtableBaseType,
                          bool firstVTable = true
                                             );

};

} // end namespace builder::vmll
} // end namespace builder

#endif
