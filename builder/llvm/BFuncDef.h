// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BFuncDef_h_
#define _builder_llvm_BFuncDef_h_

#include "model/FuncDef.h"
#include "model/Namespace.h"
#include "LLVMBuilder.h"
#include "BTypeDef.h"

namespace llvm {
    class Function;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BFuncDef);

class BFuncDef : public model::FuncDef {
public:
    // this holds the function object for the last module to request
    // it.
    llvm::Function *rep;

    // low level symbol name as used by llvm::Function *rep
    // this should be empty, unless it needs to link against an external symbol
    // when it's empty, rep->name will default to the crack canonical name,
    // which is the normal case for user defined functions in crack land
    // when it's set, rep->name will always be equal to this. it should be
    // set when we're pointing to e.g. a C function or C++ function from a
    // loaded extension
    std::string symbolName;

    // for a virtual function, this is the vtable slot position.
    unsigned vtableSlot;

    // for a virtual function, this holds the ancestor class that owns
    // the vtable pointer
    BTypeDefPtr vtableBase;

    BFuncDef(FuncDef::Flags flags, const std::string &name,
             size_t argCount
             ) :
    model::FuncDef(flags, name, argCount),
    rep(0),
    symbolName(),
    vtableSlot(0) {
    }


    /**
     * If our owner gets set, update the LLVM symbol name to reflect
     * the full canonical name
     */
    void setOwner(model::Namespace *o);

    /**
     * Returns the module-specific Function object for the function.
     */
    llvm::Function *getRep(LLVMBuilder &builder);

    virtual void *getFuncAddr(Builder &builder);

};

} // end namespace builder::vmll
} // end namespace builder

#endif
