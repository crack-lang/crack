// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>
                
#include "Ops.h"
#include "LLVMBuilder.h"

#include "BBranchPoint.h"
#include "BFieldRef.h"
#include "BResultExpr.h"
#include "BTypeDef.h"
#include "model/AllocExpr.h"
#include "model/AssignExpr.h"
#include "model/CleanupFrame.h"
#include "model/VarRef.h"

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder::mvll;

typedef spug::RCPtr<builder::mvll::BFieldRef> BFieldRefPtr;

#define UNOP(opCode) \
    model::ResultExprPtr opCode##OpCall::emit(model::Context &context) {    \
                receiver->emit(context)->handleTransient(context);          \
                                                                            \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(                         \
                        builder.lastValue,                                  \
                        BTypeDefPtr::arcast(func->returnType)->rep          \
                    );                                                      \
                                                                            \
                return new BResultExpr(this, builder.lastValue);            \
            }                                                               \

#define QUAL_BINOP(prefix, opCode, op)                                      \
  ResultExprPtr prefix##OpCall::emit(Context &context) {                    \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                                                                            \
                args[0]->emit(context)->handleTransient(context);           \
                Value *lhs = builder.lastValue;                             \
                args[1]->emit(context)->handleTransient(context);           \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(lhs,                     \
                                                   builder.lastValue        \
                                                   );                       \
                                                                            \
                return new BResultExpr(this, builder.lastValue);            \
            }                                                               \

#define BINOP(opCode, op) QUAL_BINOP(opCode, opCode, op)

// Binary Ops
BINOP(Add, "+");
BINOP(Sub, "-");
BINOP(Mul, "*");
BINOP(SDiv, "/");
BINOP(UDiv, "/");
BINOP(SRem, "%");  // Note: C'99 defines '%' as the remainder, not modulo
BINOP(URem, "%");  // the sign is that of the dividend, not divisor.
BINOP(Or, "|");
BINOP(And, "&");
BINOP(Xor, "^");
BINOP(Shl, "<<");
BINOP(LShr, ">>");
BINOP(AShr, ">>");

BINOP(ICmpEQ, "==");
BINOP(ICmpNE, "!=");
BINOP(ICmpSGT, ">");
BINOP(ICmpSLT, "<");
BINOP(ICmpSGE, ">=");
BINOP(ICmpSLE, "<=");
BINOP(ICmpUGT, ">");
BINOP(ICmpULT, "<");
BINOP(ICmpUGE, ">=");
BINOP(ICmpULE, "<=");

BINOP(FAdd, "+");
BINOP(FSub, "-");
BINOP(FMul, "*");
BINOP(FDiv, "/");
BINOP(FRem, "%");

BINOP(FCmpOEQ, "==");
BINOP(FCmpONE, "!=");
BINOP(FCmpOGT, ">");
BINOP(FCmpOLT, "<");
BINOP(FCmpOGE, ">=");
BINOP(FCmpOLE, "<=");

QUAL_BINOP(Is, ICmpEQ, "is");

// Type Conversion Ops
UNOP(SExt);
UNOP(ZExt);
UNOP(FPExt);
UNOP(SIToFP);
UNOP(UIToFP);

#define FPTRUNCOP(opCode) \
  ResultExprPtr opCode##OpCall::emit(Context &context) {                    \
                args[0]->emit(context)->handleTransient(context);           \
                                                                            \
                LLVMBuilder &builder =                                      \
                    dynamic_cast<LLVMBuilder &>(context.builder);           \
                builder.lastValue =                                         \
                    builder.builder.Create##opCode(                         \
                        builder.lastValue,                                  \
                        BTypeDefPtr::arcast(func->returnType)->rep          \
                    );                                                      \
                                                                            \
                return new BResultExpr(this, builder.lastValue);            \
            }                                                               \

// Floating Point Truncating Ops
FPTRUNCOP(FPTrunc);
FPTRUNCOP(FPToSI);
FPTRUNCOP(FPToUI);

// BinOpDef
BinOpDef::BinOpDef(TypeDef *argType,
                   TypeDef *resultType,
                   const string &name) :
                OpDef(resultType, FuncDef::noFlags, name, 2) {

                args[0] = new ArgDef(argType, "lhs");
                args[1] = new ArgDef(argType, "rhs");
            }
    

// TruncOpCall
ResultExprPtr TruncOpCall::emit(Context &context) {
    args[0]->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue =
            builder.builder.CreateTrunc(
                    builder.lastValue,
                    BTypeDefPtr::arcast(func->returnType)->rep
                    );

    return new BResultExpr(this, builder.lastValue);
}

// BitNotOpCall
ResultExprPtr BitNotOpCall::emit(Context &context) {
    args[0]->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue =
            builder.builder.CreateXor(
                    builder.lastValue,
                    ConstantInt::get(
                            BTypeDefPtr::arcast(func->returnType)->rep,
                            -1
                            )
                    );

    return new BResultExpr(this, builder.lastValue);
}

// BitNotOpDef
BitNotOpDef::BitNotOpDef(BTypeDef *resultType, const std::string &name) :
        OpDef(resultType, FuncDef::noFlags, name, 1) {
        args[0] = new ArgDef(resultType, "operand");
}
            
// LogicAndOpCall
ResultExprPtr LogicAndOpCall::emit(Context &context) {

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    // condition on lhs
    BranchpointPtr pos = builder.labeledIf(context,
                                           args[0].get(),
                                           "and_T",
                                           "and_F");
    BBranchpoint *bpos = BBranchpointPtr::arcast(pos);
    Value* oVal = builder.lastValue; // arg[0] condition value
    BasicBlock* oBlock = bpos->block2; // value block

    // now pointing to true block, emit condition of rhs in its
    // own cleanup frame (only want to do cleanups if we evaluated
    // this expression)
    context.createCleanupFrame();
    args[1].get()->emitCond(context);
    Value* tVal = builder.lastValue; // arg[1] condition value
    context.closeCleanupFrame();
    BasicBlock* tBlock = builder.block; // arg[1] value block

    // this branches us to end
    builder.emitEndIf(context, pos.get(), false);

    // now we phi for result
    PHINode* p = builder.builder.CreatePHI(
            BTypeDefPtr::arcast(context.globalData->boolType)->rep,
            "and_R");
    p->addIncoming(oVal, oBlock);
    p->addIncoming(tVal, tBlock);
    builder.lastValue = p;

    return new BResultExpr(this, builder.lastValue);
}


// LogicOrOpCall
ResultExprPtr LogicOrOpCall::emit(Context &context) {

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    // condition on lhs
    BranchpointPtr pos = builder.labeledIf(context,
                                           args[0].get(),
                                           "or_T",
                                           "or_F"
                                           );
    BBranchpoint *bpos = BBranchpointPtr::arcast(pos);
    Value *oVal = builder.lastValue; // arg[0] condition value
    BasicBlock *fBlock = bpos->block; // false block
    BasicBlock *oBlock = bpos->block2; // condition block

    // now pointing to true block, save it for phi
    BasicBlock *tBlock = builder.block;

    // repoint to false block, emit condition of rhs (in its own
    // cleanup frame, we only want to cleanup if we evaluated this
    // expression)
    builder.builder.SetInsertPoint(builder.block = fBlock);
    context.createCleanupFrame();
    args[1]->emitCond(context);
    Value *fVal = builder.lastValue; // arg[1] condition value
    context.closeCleanupFrame();
    // branch to true for phi
    builder.builder.CreateBr(tBlock);

    // pick up any changes to the fBlock
    fBlock = builder.block;

    // now jump back to true and phi for result
    builder.builder.SetInsertPoint(builder.block = tBlock);
    PHINode *p = builder.builder.CreatePHI(
            BTypeDefPtr::arcast(context.globalData->boolType)->rep,
            "or_R"
            );
    p->addIncoming(oVal, oBlock);
    p->addIncoming(fVal, fBlock);
    builder.lastValue = p;

    return new BResultExpr(this, builder.lastValue);

}

// NegOpCall
ResultExprPtr NegOpCall::emit(Context &context) {
    args[0]->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    builder.lastValue =
            builder.builder.CreateSub(
                    ConstantInt::get(
                            BTypeDefPtr::arcast(func->returnType)->rep,
                            0
                            ),
                    builder.lastValue
                    );
    return new BResultExpr(this, builder.lastValue);
}

// NegOpDef
NegOpDef::NegOpDef(BTypeDef *resultType, const std::string &name) :
        OpDef(resultType, FuncDef::noFlags, name, 1) {
    args[0] = new ArgDef(resultType, "operand");
}

// FNegOpCall
ResultExprPtr FNegOpCall::emit(Context &context) {
    args[0]->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    builder.lastValue =
            builder.builder.CreateFSub(
                    ConstantFP::get(BTypeDefPtr::arcast(func->returnType)->rep,
                                    0
                                    ),
                    builder.lastValue
                    );

    return new BResultExpr(this, builder.lastValue);
}

// FNegOpDef
FNegOpDef::FNegOpDef(BTypeDef *resultType, const std::string &name) :
        OpDef(resultType, FuncDef::noFlags, name, 1) {
    args[0] = new ArgDef(resultType, "operand");
}


// ArrayGetItemCall
ResultExprPtr ArrayGetItemCall::emit(Context &context) {
    receiver->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    Value *r = builder.lastValue;
    args[0]->emit(context)->handleTransient(context);
    Value *addr = builder.builder.CreateGEP(r, builder.lastValue);
    builder.lastValue = builder.builder.CreateLoad(addr);

    return new BResultExpr(this, builder.lastValue);
}


// ArraySetItemCall
ResultExprPtr ArraySetItemCall::emit(Context &context) {
    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    // emit the receiver
    receiver->emit(context)->handleTransient(context);
    Value *r = builder.lastValue;

    // emit the index
    args[0]->emit(context)->handleTransient(context);
    Value *i = builder.lastValue;

    // emit the rhs value
    args[1]->emit(context)->handleTransient(context);
    Value *v = builder.lastValue;

    // get the address of the index, store the value in it.
    Value *addr = builder.builder.CreateGEP(r, i);
    builder.lastValue =
            builder.builder.CreateStore(v, addr);

    return new BResultExpr(this, v);
}
    
// ArrayAllocCall
ResultExprPtr ArrayAllocCall::emit(Context &context) {
    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    // get the BTypeDef from the return type, then get the pointer
    // type out of that
    BTypeDef *retType = BTypeDefPtr::rcast(func->returnType);
    PointerType *ptrType =
            cast<PointerType>(const_cast<Type *>(retType->rep));

    // malloc based on the element type
    builder.emitAlloc(context, new
                      AllocExpr(func->returnType.get()),
                      args[0].get()
                      );

    return new BResultExpr(this, builder.lastValue);
}

// ArrayOffsetCall
ResultExprPtr ArrayOffsetCall::emit(Context &context) {
    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    args[0]->emit(context)->handleTransient(context);
    Value *base = builder.lastValue;

    args[1]->emit(context)->handleTransient(context);
    builder.lastValue =
            builder.builder.CreateGEP(base, builder.lastValue);

    return new BResultExpr(this, builder.lastValue);
}
    
// BoolOpCall
ResultExprPtr BoolOpCall::emit(Context &context) {
    // emit the receiver
    receiver->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue = 
        builder.builder.CreateICmpNE(
            builder.lastValue, 
            Constant::getNullValue(builder.lastValue->getType())
        );

    return new BResultExpr(this, builder.lastValue);
}

// FBoolOpCall
ResultExprPtr FBoolOpCall::emit(Context &context) {
    // emit the receiver
    receiver->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue =
        builder.builder.CreateFCmpONE(
            builder.lastValue,
            Constant::getNullValue(builder.lastValue->getType())
        );

    return new BResultExpr(this, builder.lastValue);
}

// VoidPtrOpCall
ResultExprPtr VoidPtrOpCall::emit(Context &context) {
    // emit the receiver
    receiver->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    builder.lastValue = builder.builder.CreateBitCast(builder.lastValue, 
                                                      builder.llvmVoidPtrType 
                                                      );

    return new BResultExpr(this, builder.lastValue);
}

// UnsafeCastCall
ResultExprPtr UnsafeCastCall::emit(Context &context) {
    // emit the argument
    args[0]->emit(context)->handleTransient(context);

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);
    BTypeDef *type = BTypeDefPtr::arcast(func->returnType);
    builder.lastValue =
            builder.builder.CreateBitCast(builder.lastValue,
                                          type->rep
                                          );

    return new BResultExpr(this, builder.lastValue);
}

// UnsafeCastDef
UnsafeCastDef::UnsafeCastDef(TypeDef *resultType) :
        OpDef(resultType, model::FuncDef::noFlags, "unsafeCast", 1) {
    args[0] = new ArgDef(resultType, "val");
}

namespace {
    LLVMBuilder &beginIncrDecr(Expr *receiver, Context &context,
                               VarRef *&ref,
                               TypeDef *type,
                               BTypeDef *&t,
                               ResultExprPtr &receiverResult,
                               Value *&receiverVal
                               ) {
        // the receiver needs to be a variable
        ref = VarRefPtr::cast(receiver);
        if (!ref)
            context.error("Integer ++ operators can only be used on variables.");
    
        receiverResult = receiver->emit(context);
        LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
        receiverVal = builder.lastValue;
        t = BTypeDefPtr::acast(type);
        return builder;
    }
    
    ResultExprPtr emitAssign(Context &context, VarRef *ref, 
                             ResultExpr *mutated
                             ) {
        // create a field assignment or variable assignment expression, as 
        // appropriate.
        AssignExprPtr assign;
        BFieldRef *fieldRef;
        if (fieldRef = BFieldRefPtr::cast(ref))
            assign = new AssignExpr(fieldRef->aggregate.get(),
                                    fieldRef->def.get(),
                                    mutated
                                    );
        else
            assign = new AssignExpr(0, ref->def.get(), mutated);
        return assign->emit(context);
    }

    inline ResultExprPtr endPreIncrDecr(Context &context, 
                                        LLVMBuilder &builder,
                                        VarRef *ref,
                                        Expr *mutatedResult,
                                        Value *mutatedVal
                                        ) {
        ResultExprPtr mutated = new BResultExpr(mutatedResult, mutatedVal);
        mutated->handleTransient(context);
        return emitAssign(context, ref, mutated.get());
    }
} // anon namespace
        

// PreIncrIntOpCall
ResultExprPtr PreIncrIntOpCall::emit(Context &context) {
    VarRef *ref;
    BTypeDef *t;
    ResultExprPtr receiverResult;
    Value *receiverVal;
    LLVMBuilder &builder = beginIncrDecr(receiver.get(), context, ref,
                                         type.get(),
                                         t,
                                         receiverResult, 
                                         receiverVal
                                         );
    receiverResult->handleTransient(context);
    builder.lastValue = builder.builder.CreateAdd(builder.lastValue, 
                                                  ConstantInt::get(t->rep, 1)
                                                  );
    return endPreIncrDecr(context, builder, ref, this, builder.lastValue);
}

// PreDecrIntOpCall
ResultExprPtr PreDecrIntOpCall::emit(Context &context) {
    VarRef *ref;
    BTypeDef *t;
    ResultExprPtr receiverResult;
    Value *receiverVal;
    LLVMBuilder &builder = beginIncrDecr(receiver.get(), context, ref,
                                         type.get(),
                                         t, 
                                         receiverResult, 
                                         receiverVal
                                         );
    receiverResult->handleTransient(context);
    builder.lastValue = builder.builder.CreateSub(builder.lastValue, 
                                                  ConstantInt::get(t->rep, 1)
                                                  );
    return endPreIncrDecr(context, builder, ref, this, builder.lastValue);
}

// PostIncrIntOpCall
ResultExprPtr PostIncrIntOpCall::emit(Context &context) {
    VarRef *ref;
    BTypeDef *t;
    ResultExprPtr receiverResult;
    Value *receiverVal;
    LLVMBuilder &builder = beginIncrDecr(receiver.get(), context, ref,
                                         type.get(), 
                                         t, 
                                         receiverResult, 
                                         receiverVal
                                         );
    Value *mutatedVal = builder.builder.CreateAdd(builder.lastValue, 
                                                  ConstantInt::get(t->rep, 1)
                                                  );
    ResultExprPtr assign = endPreIncrDecr(context, builder, ref, this, 
                                          mutatedVal
                                          );
    assign->handleTransient(context);
    builder.lastValue = receiverVal;
    return receiverResult;
}

// PostDecrIntOpCall
ResultExprPtr PostDecrIntOpCall::emit(Context &context) {
    VarRef *ref;
    BTypeDef *t;
    ResultExprPtr receiverResult;
    Value *receiverVal;
    LLVMBuilder &builder = beginIncrDecr(receiver.get(), context, ref,
                                         type.get(),
                                         t, 
                                         receiverResult, 
                                         receiverVal
                                         );
    Value *mutatedVal = builder.builder.CreateSub(builder.lastValue, 
                                                 ConstantInt::get(t->rep, 1)
                                                 );
    ResultExprPtr assign = endPreIncrDecr(context, builder, ref, this, 
                                          mutatedVal
                                          );
    assign->handleTransient(context);
    builder.lastValue = receiverVal;
    return receiverResult;
}
