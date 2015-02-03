// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010,2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_BFuncDef_h_
#define _builder_llvm_BFuncDef_h_

#include "model/FuncDef.h"
#include "model/Namespace.h"
#include "spug/check.h"
#include "LLVMBuilder.h"
#include "BTypeDef.h"

namespace llvm {
    class Function;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BFuncDef);

class BFuncDef : public model::FuncDef {
private:
    // this holds the function object for the last module to request
    // it.
    llvm::Constant *rep;

    // The function type.  We need to store this so we can produce an external
    // for the function when resolving it from another module.
    llvm::FunctionType *llvmFuncType;

    // This is the rep's module id, which is used in getRep() to verify that
    // we're using the right rep for the current module.
    int repModuleId;

public:
    // low level symbol name as used by llvm::Function *rep
    // this should be empty, unless it needs to link against an external symbol
    // when it's empty, rep->name will default to the crack canonical name,
    // which is the normal case for user defined functions in crack land
    // when it's set, rep->name will always be equal to this. it should be
    // set when we're pointing to e.g. a C function or C++ function from a
    // loaded extension
    std::string symbolName;

    // for a virtual function, this holds the ancestor class that owns
    // the vtable pointer
    BTypeDefPtr vtableBase;

    BFuncDef(FuncDef::Flags flags, const std::string &name,
             size_t argCount
             ) :
        model::FuncDef(flags, name, argCount),
        rep(0),
        llvmFuncType(0),
        repModuleId(-1),
        symbolName() {
    }

    /**
     * If our owner gets set, update the LLVM symbol name to reflect
     * the full canonical name
     */
    void setOwner(model::Namespace *o);

    /**
     * Returns the module-specific Function object for the function.
     */
    llvm::Constant *getRep(LLVMBuilder &builder);

    /**
     * Returns getRep() cast to a Function.  Aborts if the rep isn't a
     * Function object, which can only happen for an abstract method.
     */
    llvm::Function *getFuncRep(LLVMBuilder &builder);

    /**
     * Set the low-level function.
     */
    void setRep(llvm::Constant *newRep, int moduleId);

    llvm::FunctionType *getLLVMFuncType() const {
        SPUG_CHECK(llvmFuncType,
                   "Type not defined for function " << getFullName()
                   );
        return llvmFuncType;
    }

    void fixModule(llvm::Module *oldMod, llvm::Module *newMod);
    virtual void *getFuncAddr(Builder &builder);

    /**
     * If this is an external funtction (see BExtFuncDef), returns the
     * function address, otherwise returns null.
     */
    virtual void *getExtFuncAddr() const { return 0; }

};

} // end namespace builder::vmll
} // end namespace builder

#endif
