// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_llvm_BArgVarDefImpl_h_
#define _builder_llvm_BArgVarDefImpl_h_

#include "model/VarDef.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/ResultExpr.h"
#include "spug/check.h"
#include "LLVMBuilder.h"

namespace llvm {
    class Value;
    class GlobalVariable;
}

namespace model {
    class AssignExpr;
    class VarRef;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BArgVarDefImpl);

class BArgVarDefImpl : public model::VarDefImpl {
public:
    llvm::Value *rep;

    BArgVarDefImpl(llvm::Value *rep) : rep(rep) {}

    virtual model::ResultExprPtr emitRef(model::Context &context,
                                         model::VarRef *var
                                        );
    
    virtual model::ResultExprPtr
            emitAssignment(model::Context &context,
                           model::AssignExpr *assign);

    virtual void emitAddr(model::Context &context, model::VarRef *var);

    model::VarDefImplPtr promote(LLVMBuilder &builder, model::ArgDef *arg);
    
    virtual bool hasInstSlot() const;
    virtual int getInstSlot() const;
    virtual bool isInstVar() const;
};


// generates references for memory variables (globals and instance vars)
SPUG_RCPTR(BMemVarDefImpl);
class BMemVarDefImpl : public model::VarDefImpl {
public:

    virtual model::ResultExprPtr emitRef(model::Context &context,
                                         model::VarRef *var);

    virtual model::ResultExprPtr
            emitAssignment(model::Context &context,
                           model::AssignExpr *assign);

    virtual void emitAddr(model::Context &context, model::VarRef *var);

    virtual llvm::Value *getRep(LLVMBuilder &builder) = 0;

    virtual bool hasInstSlot() const;
    virtual int getInstSlot() const;
    virtual bool isInstVar() const;
};

SPUG_RCPTR(BHeapVarDefImpl)
class BHeapVarDefImpl : public BMemVarDefImpl {
public:
    llvm::Value *rep;

    BHeapVarDefImpl(llvm::Value *rep) : rep(rep) {}

    virtual llvm::Value *getRep(LLVMBuilder &builder) {
        return rep;
    }
};

SPUG_RCPTR(BGlobalVarDefImpl);
class BGlobalVarDefImpl : public BMemVarDefImpl {
private:
    // global var rep's are module specific, this variable points to the
    // value for the module most recently evaluated by getRep().
    llvm::GlobalVariable *rep;
    llvm::PointerType *llvmType;
    std::string name;
    bool constant;
    int repModuleId;
public:

    BGlobalVarDefImpl(llvm::GlobalVariable *rep, int repModuleId);

    virtual llvm::Value *getRep(LLVMBuilder &builder);
    
    llvm::PointerType *getLLVMType() const {
        return llvmType;
    }
    
    std::string getName() const { return name; }
    bool isConstant() const { return constant; }
    
    void setRep(llvm::GlobalVariable *newRep, int newRepModuleId) {
        rep = newRep;
        repModuleId = newRepModuleId;
    }
    
    void fixModule(llvm::Module *oldMod, llvm::Module *newMod);
};

// these are actually only used for function implementations.
SPUG_RCPTR(BConstDefImpl);
class BConstDefImpl : public model::VarDefImpl {
private:
    // we use a raw pointer here because a FuncDef owns its impl.
    BFuncDef *func;
    llvm::Function *rep;

public:

    BConstDefImpl(BFuncDef *func, llvm::Function *rep) : 
        func(func), rep(rep) {}

    virtual model::ResultExprPtr emitRef(model::Context &context,
                                         model::VarRef *var);

    virtual model::ResultExprPtr
            emitAssignment(model::Context &context,
                           model::AssignExpr *assign) {
        assert(false && "assignment to a constant");
        return 0;
    }

    virtual void emitAddr(model::Context &context, model::VarRef *var);

    virtual bool hasInstSlot() const;
    virtual int getInstSlot() const;
    virtual bool isInstVar() const;
    
    void fixModule(llvm::Module *oldMod, llvm::Module *newMod);
};

SPUG_RCPTR(BFieldDefImpl);

// Base class for variable implementations that are offsets from a base 
// pointer.
class BFieldDefImpl : public model::VarDefImpl {
    public:
        virtual model::ResultExprPtr emitRef(model::Context &context,
                                             model::VarRef *var
                                             ) {
            assert(false &&
                    "attempting to emit a direct reference to a instance "
                    "variable."
                   );
        }
    
        virtual model::ResultExprPtr emitAssignment(model::Context &context,
                                                    model::AssignExpr *assign
                                                    ) {
            assert(false &&
                    "attempting to assign a direct reference to a instance "
                    "variable."
                   );
        }

        // emit assignment of the field in the aggregate from the value.
        virtual void emitFieldAssign(llvm::IRBuilder<> &builder,
                                     llvm::Value *aggregate,
                                     llvm::Value *value
                                     ) = 0;

        // emit a field reference.
        virtual llvm::Value *emitFieldRef(llvm::IRBuilder<> &builder,
                                          llvm::Type *fieldType,
                                          llvm::Value *aggregate
                                          ) = 0;

        virtual void emitAddr(model::Context &context, model::VarRef *var);
        
        virtual llvm::Value *emitFieldAddr(llvm::IRBuilder<> &builder,
                                           llvm::Type *fieldType,
                                           llvm::Value *aggregate
                                           ) = 0;
};

SPUG_RCPTR(BInstVarDefImpl);

// Impl object for instance variables.  These should never be used to emit
// instance variables, so when used they just raise an assertion error.
class BInstVarDefImpl : public BFieldDefImpl {
    public:
        unsigned index;
        BInstVarDefImpl(unsigned index) : index(index) {}

        virtual void emitFieldAssign(llvm::IRBuilder<> &builder,
                                     llvm::Value *aggregate,
                                     llvm::Value *value
                                     );

        virtual llvm::Value *emitFieldRef(llvm::IRBuilder<> &builder,
                                          llvm::Type *fieldType,
                                          llvm::Value *aggregate
                                          );

        virtual llvm::Value *emitFieldAddr(llvm::IRBuilder<> &builder,
                                           llvm::Type *fieldType,
                                           llvm::Value *aggregate
                                           );

        virtual bool hasInstSlot() const;
        virtual int getInstSlot() const;
        virtual bool isInstVar() const;
};

// Implementation for "offset fields."  These are used to access structure 
// fields in extension objects, where we need fine grain control over where 
// the field is located.
class BOffsetFieldDefImpl : public BFieldDefImpl {
    public:
        size_t offset;
        BOffsetFieldDefImpl(size_t offset) : offset(offset) {}

        virtual void emitFieldAssign(llvm::IRBuilder<> &builder,
                                     llvm::Value *aggregate,
                                     llvm::Value *value
                                     );

        virtual llvm::Value *emitFieldRef(llvm::IRBuilder<> &builder,
                                          llvm::Type *fieldType,
                                          llvm::Value *aggregate
                                          );

        virtual llvm::Value *emitFieldAddr(llvm::IRBuilder<> &builder,
                                           llvm::Type *fieldType,
                                           llvm::Value *aggregate
                                           );

        virtual bool hasInstSlot() const;
        virtual int getInstSlot() const;
        virtual bool isInstVar() const;
};


} // end namespace builder::vmll
} // end namespace builder

#endif
