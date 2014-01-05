// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "VarDefs.h"

#include "BResultExpr.h"
#include "BTypeDef.h"

#include "spug/check.h"
#include "model/AssignExpr.h"
#include "model/VarRef.h"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Function.h>

using namespace llvm;
using namespace model;
using namespace builder::mvll;

// BArgVarDefImpl

VarDefImplPtr BArgVarDefImpl::promote(LLVMBuilder &builder, ArgDef *arg) {
    Value *var;
    BHeapVarDefImplPtr localVar =
        builder.createLocalVar(BTypeDefPtr::arcast(arg->type),
                               var,
                               "",
                               NULL,
                               rep
                               );
    return localVar;
}

ResultExprPtr BArgVarDefImpl::emitRef(Context &context,
                                      VarRef *var
                                      ) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.emitArgVarRef(context, rep);

    return new BResultExpr((Expr*)var, b.lastValue);
}

ResultExprPtr BArgVarDefImpl::emitAssignment(Context &context, 
                                             AssignExpr *assign
                                             ) {
    // This should never happen - arguments except for "this" all get promoted 
    // to local variables at the start of the function.
    assert(0 && "attempting to emit an argument assignment");
}

void BArgVarDefImpl::emitAddr(Context &context, VarRef *var) {
    SPUG_CHECK(false,
               "Attempting to emit the address of argument variable " <<
                var->def->name
               );
}

bool BArgVarDefImpl::hasInstSlot() const { return false; }
int BArgVarDefImpl::getInstSlot() const { return -1; }

// BMemVarDefImpl
ResultExprPtr BMemVarDefImpl::emitRef(Context &context, VarRef *var) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.emitMemVarRef(context, getRep(b));
    return new BResultExpr((Expr*)var, b.lastValue);
}

ResultExprPtr BMemVarDefImpl::emitAssignment(Context &context, AssignExpr *assign) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    ResultExprPtr result = assign->value->emit(context);
    Value *exprVal = b.lastValue;
    b.narrow(assign->value->type.get(), assign->var->type.get());
    b.builder.CreateStore(b.lastValue, getRep(b));
    result->handleAssignment(context);
    b.lastValue = exprVal;

    return new BResultExpr(assign, exprVal);
}

void BMemVarDefImpl::emitAddr(Context &context, VarRef *var) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    b.lastValue = getRep(b);
}    

bool BMemVarDefImpl::hasInstSlot() const { return false; }
int BMemVarDefImpl::getInstSlot() const { return -1; }

// BGlobalVarDefImpl
Value * BGlobalVarDefImpl::getRep(LLVMBuilder &builder) {
    if (rep->getParent() != builder.module)
        rep = builder.getModVar(this, this->rep);
    return rep;
}

// BConstDefImpl
ResultExprPtr BConstDefImpl::emitRef(Context &context, VarRef *var) {
    LLVMBuilder &b =
            dynamic_cast<LLVMBuilder &>(context.builder);
    if (rep->getParent() != b.module)
        rep = b.getModFunc(func, rep);
    b.lastValue = rep;
    return new BResultExpr((Expr*)var, b.lastValue);
}

void BConstDefImpl::emitAddr(Context &context, VarRef *var) {
    SPUG_CHECK(false,
               "Attempting to emit the address of a constant variable " <<
                var->def->name
               );
}

bool BConstDefImpl::hasInstSlot() const { return false; }
int BConstDefImpl::getInstSlot() const { return -1; }

// BFieldDefImpl

void BFieldDefImpl::emitAddr(model::Context &context, model::VarRef *var) {
    SPUG_CHECK(false,
               "Attempting to directly emit the address of field " <<
                var->def->name
               );
}

// BInstVarDefImpl
void BInstVarDefImpl::emitFieldAssign(IRBuilder<> &builder, Value *aggregate,
                                      Value *value
                                      ) {
    Value *fieldPtr = builder.CreateStructGEP(aggregate, index);
    builder.CreateStore(value, fieldPtr);
}

Value *BInstVarDefImpl::emitFieldRef(IRBuilder<> &builder, 
                                     Type *fieldType,
                                     Value *aggregate
                                     ) {
    Value *fieldPtr = builder.CreateStructGEP(aggregate, index);
    return builder.CreateLoad(fieldPtr);
}

Value *BInstVarDefImpl::emitFieldAddr(IRBuilder<> &builder, Type *fieldType,
                                      Value *aggregate
                                      ) {
    return builder.CreateStructGEP(aggregate, index);
}                                    

bool BInstVarDefImpl::hasInstSlot() const { return true; }
int BInstVarDefImpl::getInstSlot() const { return index; }

// BOffsetFieldDefImpl
void BOffsetFieldDefImpl::emitFieldAssign(IRBuilder<> &builder,
                                          Value *aggregate,
                                          Value *value
                                          ) {
    // cast to a byte pointer, GEP to the offset
    Value *fieldPtr = builder.CreateBitCast(aggregate, 
                                            builder.getInt8Ty()->getPointerTo()
                                            );
    fieldPtr = builder.CreateConstGEP1_32(fieldPtr, offset);
    Type *fieldPtrType = value->getType()->getPointerTo();
    builder.CreateStore(value, builder.CreateBitCast(fieldPtr, fieldPtrType));
}

Value *BOffsetFieldDefImpl::emitFieldRef(IRBuilder<> &builder,
                                         Type *fieldType,
                                         Value *aggregate
                                         ) {
    // cast to a byte pointer, GEP to the offset
    Value *fieldPtr = builder.CreateBitCast(aggregate, 
                                            builder.getInt8Ty()->getPointerTo()
                                            );
    fieldPtr = builder.CreateConstGEP1_32(fieldPtr, offset);
    Type *fieldPtrType = fieldType->getPointerTo();
    return builder.CreateLoad(builder.CreateBitCast(fieldPtr, fieldPtrType));
}
    
Value *BOffsetFieldDefImpl::emitFieldAddr(IRBuilder<> &builder, 
                                          Type *fieldType,
                                          Value *aggregate
                                          ) {
    Value *fieldPtr = builder.CreateBitCast(aggregate,
                                            builder.getInt8Ty()->getPointerTo()
                                            );
    fieldPtr = builder.CreateConstGEP1_32(fieldPtr, offset);
    Type *fieldPtrType = fieldType->getPointerTo();
    return builder.CreateBitCast(fieldPtr, fieldPtrType);
}

bool BOffsetFieldDefImpl::hasInstSlot() const { return false; }

int BOffsetFieldDefImpl::getInstSlot() const {
    SPUG_CHECK(false, "Can't serialize offset variables yet.");
}
