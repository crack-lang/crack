// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>
                
#include "Ops.h"
#include "LLVMBuilder.h"

#include "BBranchPoint.h"
#include "BFieldRef.h"
#include "BResultExpr.h"
#include "BTypeDef.h"
#include "BFuncDef.h"
#include "BFuncPtr.h"
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
        if (receiver)                                                       \
            receiver->emit(context)->handleTransient(context);              \
        else                                                                \
            args[0]->emit(context)->handleTransient(context);               \
        LLVMBuilder &builder =                                              \
            dynamic_cast<LLVMBuilder &>(context.builder);                   \
        builder.lastValue =                                                 \
            builder.builder.Create##opCode(                                 \
                builder.lastValue,                                          \
                BTypeDefPtr::arcast(func->returnType)->rep                  \
            );                                                              \
                                                                            \
        return new BResultExpr(this, builder.lastValue);                    \
    }

#define QUAL_BINOP(prefix, opCode, op)                                      \
    ResultExprPtr prefix##OpCall::emit(Context &context) {                  \
        LLVMBuilder &builder =                                              \
            dynamic_cast<LLVMBuilder &>(context.builder);                   \
        int arg = 0;                                                        \
        if (receiver)                                                       \
            receiver->emit(context)->handleTransient(context);              \
        else                                                                \
            args[arg++]->emit(context)->handleTransient(context);           \
        Value *lhs = builder.lastValue;                                     \
        args[arg]->emit(context)->handleTransient(context);                 \
        builder.lastValue =                                                 \
            builder.builder.Create##opCode(lhs,                             \
                                           builder.lastValue                \
                                           );                               \
                                                                            \
        return new BResultExpr(this, builder.lastValue);                    \
    }                                                                       \

// reverse binary operators, must be run as methods.
#define REV_BINOP(opCode) \
    ResultExprPtr opCode##ROpCall::emit(Context &context) {                 \
        LLVMBuilder &builder =                                              \
            dynamic_cast<LLVMBuilder &>(context.builder);                   \
        receiver->emit(context)->handleTransient(context);                  \
        Value *rhs = builder.lastValue;                                     \
        args[0]->emit(context)->handleTransient(context);                   \
        builder.lastValue =                                                 \
            builder.builder.Create##opCode(builder.lastValue,               \
                                           rhs                              \
                                           );                               \
                                                                            \
        return new BResultExpr(this, builder.lastValue);                    \
    }                                                                       \

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
REV_BINOP(Add)
REV_BINOP(Sub)
REV_BINOP(Mul)
REV_BINOP(SDiv)
REV_BINOP(UDiv)
REV_BINOP(SRem)
REV_BINOP(URem)
REV_BINOP(Or)
REV_BINOP(And)
REV_BINOP(Xor)
REV_BINOP(Shl)
REV_BINOP(LShr)
REV_BINOP(AShr)

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
REV_BINOP(ICmpEQ)
REV_BINOP(ICmpNE)
REV_BINOP(ICmpSGT)
REV_BINOP(ICmpSLT)
REV_BINOP(ICmpSGE)
REV_BINOP(ICmpSLE)
REV_BINOP(ICmpUGT)
REV_BINOP(ICmpULT)
REV_BINOP(ICmpUGE)
REV_BINOP(ICmpULE)

BINOP(FAdd, "+");
BINOP(FSub, "-");
BINOP(FMul, "*");
BINOP(FDiv, "/");
BINOP(FRem, "%");
REV_BINOP(FAdd)
REV_BINOP(FSub)
REV_BINOP(FMul)
REV_BINOP(FDiv)
REV_BINOP(FRem)

BINOP(FCmpOEQ, "==");
BINOP(FCmpONE, "!=");
BINOP(FCmpOGT, ">");
BINOP(FCmpOLT, "<");
BINOP(FCmpOGE, ">=");
BINOP(FCmpOLE, "<=");
REV_BINOP(FCmpOEQ)
REV_BINOP(FCmpONE)
REV_BINOP(FCmpOGT)
REV_BINOP(FCmpOLT)
REV_BINOP(FCmpOGE)
REV_BINOP(FCmpOLE)

QUAL_BINOP(Is, ICmpEQ, "is");

// Type Conversion Ops
UNOP(SExt);
UNOP(ZExt);
UNOP(FPExt);
UNOP(SIToFP);
UNOP(UIToFP);

#define FPTRUNCOP(opCode) \
    ResultExprPtr opCode##OpCall::emit(Context &context) {          \
        if (receiver)                                               \
            receiver->emit(context)->handleTransient(context);      \
        else                                                        \
            args[0]->emit(context)->handleTransient(context);       \
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
                   const string &name,
                   bool isMethod,
                   bool reversed
                   ) :
    OpDef(resultType, 
          (isMethod ? FuncDef::method : FuncDef::noFlags) |
           (reversed ? FuncDef::reverse : FuncDef::noFlags), 
          name, 
          isMethod ? 1 : 2
          ) {

    int arg = 0;
    if (!isMethod)
        args[arg++] = new ArgDef(argType, "lhs");
    args[arg] = new ArgDef(argType, "rhs");
}
    

// TruncOpCall
ResultExprPtr TruncOpCall::emit(Context &context) {
    if (receiver)
        receiver->emit(context)->handleTransient(context);
    else
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

// NoOpCall
ResultExprPtr NoOpCall::emit(Context &context) {
    if (receiver)
        return receiver->emit(context);
    else
        return args[0]->emit(context);
}

// BitNotOpCall
ResultExprPtr BitNotOpCall::emit(Context &context) {
    if (receiver)
        receiver->emit(context)->handleTransient(context);
    else
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
BitNotOpDef::BitNotOpDef(BTypeDef *resultType, const std::string &name,
                         bool isMethod
                         ) :
    OpDef(resultType, isMethod ? FuncDef::method :FuncDef::noFlags, name, 
          isMethod ? 0 : 1
          ) {
    if (!isMethod)
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
    BasicBlock* tBlock = builder.builder.GetInsertBlock(); // arg[1] val block

    // this branches us to end
    builder.emitEndIf(context, pos.get(), false);

    // now we phi for result
    PHINode* p = builder.builder.CreatePHI(
            BTypeDefPtr::arcast(context.construct->boolType)->rep,
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
    BasicBlock *tBlock = builder.builder.GetInsertBlock();

    // repoint to false block, emit condition of rhs (in its own
    // cleanup frame, we only want to cleanup if we evaluated this
    // expression)
    builder.builder.SetInsertPoint(fBlock);
    context.createCleanupFrame();
    args[1]->emitCond(context);
    Value *fVal = builder.lastValue; // arg[1] condition value
    context.closeCleanupFrame();
    // branch to true for phi
    builder.builder.CreateBr(tBlock);

    // pick up any changes to the fBlock
    fBlock = builder.builder.GetInsertBlock();

    // now jump back to true and phi for result
    builder.builder.SetInsertPoint(tBlock);
    PHINode *p = builder.builder.CreatePHI(
            BTypeDefPtr::arcast(context.construct->boolType)->rep,
            "or_R"
            );
    p->addIncoming(oVal, oBlock);
    p->addIncoming(fVal, fBlock);
    builder.lastValue = p;

    return new BResultExpr(this, builder.lastValue);

}

// NegOpCall
ResultExprPtr NegOpCall::emit(Context &context) {
    if (receiver)
        receiver->emit(context)->handleTransient(context);
    else
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
NegOpDef::NegOpDef(BTypeDef *resultType, const std::string &name,
                   bool isMethod
                   ) :
        OpDef(resultType, isMethod ? FuncDef::method : FuncDef::noFlags, name, 
              isMethod ? 0 : 1
              ) {
    if (!isMethod)
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

// FunctionPtrCall
ResultExprPtr FunctionPtrCall::emit(Context &context) {

    LLVMBuilder &builder =
            dynamic_cast<LLVMBuilder &>(context.builder);

    receiver->emit(context)->handleTransient(context);
    Value *fptr = builder.lastValue;

    // generate a function pointer call by passing on arguments
    // from oper call to llvm function pointer
    BFuncPtrPtr bfp = new BFuncPtr(fptr, args.size());
    bfp->returnType = func->returnType;
    bfp->args.assign(func->args.begin(), func->args.end());
    FuncCallPtr fc = new FuncCall(bfp.get());
    fc->args.assign(args.begin(), args.end());

    builder.emitFuncCall(context, fc.get());
    return new BResultExpr(this, builder.lastValue);

}

// FunctionPtrOpDef
model::FuncCallPtr FunctionPtrOpDef::createFuncCall() {
    return new FunctionPtrCall(this);
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
    builder.narrow(args[1]->type.get(), func->args[1]->type.get());
    Value *v = builder.lastValue;

    // get the address of the index, store the value in it.
    Value *addr = builder.builder.CreateGEP(r, i);
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
    const PointerType *ptrType =
            cast<const PointerType>(retType->rep);

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

    receiver->emit(context)->handleTransient(context);
    Value *base = builder.lastValue;

    args[0]->emit(context)->handleTransient(context);
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

// PtrToIntOpCall
ResultExprPtr PtrToIntOpCall::emit(Context &context) {
    args[0]->emit(context)->handleTransient(context);
    
    LLVMBuilder &builder =
        dynamic_cast<LLVMBuilder &>(context.builder);
    BTypeDef *type = BTypeDefPtr::arcast(func->returnType);
    builder.lastValue = builder.builder.CreatePtrToInt(builder.lastValue,
                                                       type->rep
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
