// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BTypeDef_h_
#define _builder_llvm_BTypeDef_h_

#include "model/TypeDef.h"
#include "VTableBuilder.h"
#include <llvm/Type.h>

#include <map>
#include <string>
#include <vector>

namespace llvm {
    class Type;
    class Constant;
    class ExecutionEngine;
    class GlobalVariable;
}

namespace builder {
namespace mvll {

class PlaceholderInstruction;

SPUG_RCPTR(BTypeDef);

// (RCPTR defined above)
class BTypeDef : public model::TypeDef {
public:
    unsigned fieldCount;
    llvm::GlobalVariable *classInst;
    const llvm::Type *rep;
    unsigned nextVTableSlot;
    std::vector<PlaceholderInstruction *> placeholders;

    // mapping from base types to their vtables.
    std::map<BTypeDef *, llvm::Constant *> vtables;
    llvm::PATypeHolder firstVTableType;

    BTypeDef(TypeDef *metaType, const std::string &name,
             const llvm::Type *rep,
             bool pointer = false,
             unsigned nextVTableSlot = 0
             ) :
        model::TypeDef(metaType, name, pointer),
        fieldCount(0),
        classInst(0),
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

    /**
     * Add a base class to the type.
     */
    void addBaseClass(BTypeDef *base);
    
    /**
     * register a placeholder instruction to be replaced when the class is 
     * completed.
     */
    void addPlaceholder(PlaceholderInstruction *inst);
    
    /**
     * Find the ancestor with our first vtable.
     */
    BTypeDef *findFirstVTable(BTypeDef *vtableBaseType);
    
    /**
     * Get the global variable, creating an extern instance in the module if 
     * it lives in another module.
     * @param execEng the execution engine (may be null if this is unknown or 
     *  there is no execution engine)
     */
    llvm::GlobalVariable *getClassInstRep(llvm::Module *module,
                                          llvm::ExecutionEngine *execEng
                                          );
};

} // end namespace builder::vmll
} // end namespace builder

#endif
