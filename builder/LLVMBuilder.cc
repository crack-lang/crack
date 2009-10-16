
#include "LLVMBuilder.h"

// LLVM includes
#include <stddef.h>
#include <stdlib.h>
#include "llvm/LinkAllPasses.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/PassManager.h"
#include "llvm/CallingConv.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetData.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <model/ArgDef.h>
#include <model/Branchpoint.h>
#include <model/Context.h>
#include <model/FuncDef.h>
#include <model/FuncCall.h>
#include <model/IntConst.h>
#include <model/StrConst.h>
#include <model/TypeDef.h>
#include <model/VarDef.h>
#include <model/VarRef.h>


using namespace builder;

using namespace std;
using namespace llvm;
using namespace model;
typedef model::FuncCall::ExprVector ExprVector;

namespace {

    SPUG_RCPTR(BFuncDef);

    class BFuncDef : public model::FuncDef {
        public:
            llvm::Function *rep;
            BFuncDef(const char *name, size_t argCount) :
                model::FuncDef(name, argCount) {
            }
    };
    
    SPUG_RCPTR(BTypeDef)

    class BTypeDef : public model::TypeDef {
        public:
            const llvm::Type *rep;
            BTypeDef(const char *name, const llvm::Type *rep) :
                model::TypeDef(name),
                rep(rep) {
            }
    };
    
    SPUG_RCPTR(BVarDef);

    class BVarDef : public VarDef {
        public:
            llvm::Value *rep;
            BVarDef(const TypeDefPtr type, const string &name, Value *rep) :
                VarDef(type, name),
                rep(rep) {
            }
    };
    
    SPUG_RCPTR(BStrConst);

    class BStrConst : public model::StrConst {
        public:
            // XXX need more specific type?
            llvm::Value *rep;
            BStrConst(const TypeDefPtr &type, const std::string &val) :
                StrConst(type, val),
                rep(0) {
            }
    };
    
    class BIntConst : public model::IntConst {
        public:
            llvm::Value *rep;
            BIntConst(const TypeDefPtr &type, long val) :
                IntConst(type, val),
                rep(llvm::ConstantInt::get(llvm::Type::Int32Ty, val)) {
            }
    };
    
    SPUG_RCPTR(BBranchpoint);

    class BBranchpoint : public model::Branchpoint {
        public:
            BasicBlock *block;
            
            BBranchpoint(BasicBlock *block) : block(block) {}
    };

} // anon namespace

LLVMBuilder::LLVMBuilder() :
    module(0),
    func(0),
    block(0),
    lastValue(0) {
}

void LLVMBuilder::emitFuncCall(model::Context &context,
                               const model::FuncDefPtr &funcDef, 
                               const model::FuncCall::ExprVector &args) {
                    
    // get the LLVM arg list from the argument expressions
    vector<Value*> valueArgs;
    for (ExprVector::const_iterator iter = args.begin(); iter < args.end(); 
         ++iter) {
        (*iter)->emit(context);
        valueArgs.push_back(lastValue);
    }
    
    lastValue =
        builder.CreateCall(BFuncDefPtr::dcast(funcDef)->rep, valueArgs.begin(),
                           valueArgs.end(),
                           "tmp"
                           );
}

void LLVMBuilder::emitStrConst(model::Context &context, 
                               const StrConstPtr &val) {
    BStrConstPtr bval = BStrConstPtr::dcast(val);
    // if the global string hasn't been defined yet, create it
    if (!bval->rep) {
        bval->rep = builder.CreateGlobalStringPtr(val->val.c_str());
    }
    lastValue = bval->rep;
}

void LLVMBuilder::emitIntConst(model::Context &context, const IntConst &val) {
    lastValue = dynamic_cast<const BIntConst &>(val).rep;
}

BranchpointPtr LLVMBuilder::emitIf(model::Context &context,
                                   const model::ExprPtr &cond) {
    // stash the current block and the "false condition" block in the result 
    // branchpoint and create a new block for the condition
    BBranchpointPtr result = new BBranchpoint(BasicBlock::Create("cond_false",
                                                                 func
                                                                 )
                                              );
    block = BasicBlock::Create("cond_true", func);

    cond->emit(context);
    // XXX I think we need a "conditional" type so we don't have to convert 
    // everything to a boolean and then check for non-zero.
    BTypeDefPtr boolType =
        BTypeDefPtr::dcast(context.globalData->boolType);
    Value *comparison =
        builder.CreateICmpNE(lastValue, Constant::getNullValue(boolType->rep));
    builder.CreateCondBr(comparison, block, result->block);
    
    // repoint to the new ("if true") block
    builder.SetInsertPoint(block);
    return BranchpointPtr::ucast(result);
}

BranchpointPtr LLVMBuilder::emitElse(model::Context &context,
                                     const model::BranchpointPtr &pos) {
    BBranchpointPtr bpos = BBranchpointPtr::dcast(pos);

    // create a block to come after the else and jump to it from the current 
    // "if true" block.
    BasicBlock *falseBlock = bpos->block;
    bpos->block = BasicBlock::Create("cond_end", func);
    builder.CreateBr(bpos->block);
    
    // new block is the "false" condition
    block = falseBlock;
    builder.SetInsertPoint(block);
    return pos;
}
        
void LLVMBuilder::emitEndIf(model::Context &context,
                            const model::BranchpointPtr &pos) {
    BBranchpointPtr bpos = BBranchpointPtr::dcast(pos);

    // branch from the current block to the next block
    builder.CreateBr(bpos->block);

    // new block is the next block
    block = BBranchpointPtr::dcast(pos)->block;
    builder.SetInsertPoint(block);
}

Value *emitGEP(IRBuilder<> &builder, Value *obj) {
    Value *zero = llvm::ConstantInt::get(llvm::Type::Int32Ty, 0);
    Value *gepArgs[] = { zero, zero };
    return builder.CreateGEP(obj, zero);
}
    
VarDefPtr LLVMBuilder::emitVarDef(Context &context, const TypeDefPtr &type,
                                  const string &name,
                                  const ExprPtr &initializer,
                                  bool staticScope
                                  ) {
    // do initializion
    if (initializer)
        initializer->emit(context);
    else
        type->defaultInitializer->emit(context);
    
    // XXX use InternalLinkage for variables starting with _ (I think that 
    // might work)
    BTypeDef *tp = dynamic_cast<BTypeDef *>(type.obj);
    
    Value *var = 0;
    switch (context.scope) {

        case Context::instance:
            // class statics share the same context as instance variables: 
            // they are distinguished from instance variables by their 
            // declaration and are equivalent to module scoped globals in the 
            // way they are emitted, so if the staticScope flag is set we want 
            // to fall through to module scope
            if (!staticScope) {
                assert(false && "XXX write instance variable emitter.");
                break;
            }
                
        case Context::module:
            var = new GlobalVariable(tp->rep,
                                    false, // isConstant
                                    GlobalValue::ExternalLinkage, // linkage tp
                                    
                                    // initializer - this needs to be provided 
                                    // or the global will be treated as an 
                                    // extern.
                                    Constant::getNullValue(tp->rep),
                                    name,
                                    module
                                    );
            break;

        case Context::local:
            var = builder.CreateAlloca(tp->rep, 0);
            break;
        
        default:
            assert(false && "invalid context value!");
    }
    
    // allocate the variable and assign it
    lastValue = builder.CreateStore(lastValue, var);
    
    // create the definition object.
    return new BVarDef(type, name, var);
}
 
void LLVMBuilder::emitVarRef(model::Context &context,
                             const model::VarRef &var
                             ) {
    BVarDefPtr def = BVarDefPtr::dcast(var.def);
    lastValue = builder.CreateLoad(def->rep);
}
                                
void LLVMBuilder::createModule(const char *name) {
    assert(!module);
    module = new llvm::Module(name);
    llvm::Constant *c =
        module->getOrInsertFunction("__main__", llvm::Type::VoidTy, NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create("__main__", func);
    builder.SetInsertPoint(block);
}

void LLVMBuilder::closeModule() {
    assert(module);
    builder.CreateRetVoid();
    llvm::verifyModule(*module, llvm::PrintMessageAction);
    
    // create the execution engine
    execEng = llvm::ExecutionEngine::create(module );

    // optimize
    llvm::PassManager passMan;

    // Set up the optimizer pipeline.  Start with registering info about how 
    // the target lays out data structures.
    passMan.add(new llvm::TargetData(*execEng->getTargetData()));
    // Promote allocas to registers.
    passMan.add(createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    passMan.add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    passMan.add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    passMan.add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    passMan.add(llvm::createCFGSimplificationPass());
    
    passMan.run(*module);
}    

model::StrConstPtr LLVMBuilder::createStrConst(model::Context &context,
                                               const std::string &val) {
    return new BStrConst(context.globalData->byteptrType, val);
}

IntConstPtr LLVMBuilder::createIntConst(model::Context &context, long val) {
    // XXX probably need to consider the simplest type that the constant can 
    // fit into (compatibility rules will allow us to coerce it into another 
    // type)
    return new BIntConst(context.globalData->int32Type, val);
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(const string &funcName) {
    // XXX need a function type.
    return new FuncCall(0, funcName);
}
    
model::FuncDefPtr LLVMBuilder::createFuncDef(const char *name) {
    return new BFuncDef(name, 0);
}

VarRefPtr LLVMBuilder::createVarRef(const VarDefPtr &varDef) {
    return new VarRef(varDef);
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;
    // create the basic types
    llvm::Type *llvmBytePtrType = 
        llvm::PointerType::getUnqual(llvm::IntegerType::get(8));
    gd->byteptrType = new BTypeDef("byteptr", llvmBytePtrType);
    gd->byteptrType->defaultInitializer = createStrConst(context, "");
    context.addDef(gd->byteptrType);
    
    const llvm::Type *llvmInt32Type = llvm::IntegerType::get(32);
    gd->int32Type = new BTypeDef("int32", llvmInt32Type);
    gd->int32Type->defaultInitializer = createIntConst(context, 0);
    context.addDef(gd->int32Type);
    
    // XXX using bool = int32 for now
    gd->boolType = gd->int32Type;
    
    // create "int print(String)"
    vector<const llvm::Type *> args(1);
    args[0] = llvmBytePtrType;
    FunctionType *funcType = FunctionType::get(llvm::IntegerType::get(32),
                                               args,
                                               false
                                               );
    llvm::Function *printFunc = llvm::Function::Create(funcType,
                                                       Function::ExternalLinkage,
                                                       "printf",
                                                       module
                                                       );
    
    BFuncDefPtr funcDef = new BFuncDef("print", 1);
    funcDef->rep = printFunc;
    funcDef->args[0] = new ArgDef(gd->byteptrType, "text");
    context.addDef(VarDefPtr::ucast(funcDef));
}

void LLVMBuilder::run() {
//    PassManager passMan;
//    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
//    passMan.run(*module);
    
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}
