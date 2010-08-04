// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BArgVarDefImpl_h_
#define _builder_llvm_BArgVarDefImpl_h_

#include "model/VarDef.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
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

    virtual llvm::Value *getRep(LLVMBuilder &builder) = 0;

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
public:
    llvm::GlobalVariable *rep;

    BGlobalVarDefImpl(llvm::GlobalVariable *rep) : rep(rep) {}

    virtual llvm::Value *getRep(LLVMBuilder &builder);

};

class BConstDefImpl : public model::VarDefImpl {
public:
    llvm::Constant *rep;

    BConstDefImpl(llvm::Constant *rep) : rep(rep) {}

    virtual model::ResultExprPtr emitRef(model::Context &context,
                                         model::VarRef *var);

    virtual model::ResultExprPtr
            emitAssignment(model::Context &context,
                           model::AssignExpr *assign) {
        assert(false && "assignment to a constant");
        return 0;
    }

};

SPUG_RCPTR(BInstVarDefImpl);

// Impl object for instance variables.  These should never be used to emit
// instance variables, so when used they just raise an assertion error.
class BInstVarDefImpl : public model::VarDefImpl {
public:
    unsigned index;
    BInstVarDefImpl(unsigned index) : index(index) {}
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
};

} // end namespace builder::vmll
} // end namespace builder

#endif
