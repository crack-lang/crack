
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
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetData.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <model/ArgDef.h>
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
            BStrConst(const std::string &val) :
                StrConst(val),
                rep(0) {
            }
    };
    
    class BIntConst : public model::IntConst {
        public:
            llvm::Value *rep;
            BIntConst(long val) :
                IntConst(val),
                rep(llvm::ConstantInt::get(llvm::Type::Int32Ty, val)) {
            }
    };

} // anon namespace

LLVMBuilder::LLVMBuilder() :
    module(0),
    func(0),
    block(0),
    lastValue(0) {
}

void LLVMBuilder::emitFuncCall(model::Context &context,
                               const model::FuncDefPtr &func, 
                               const model::FuncCall::ExprVector &args) {
                    
    // get the LLVM arg list from the argument expressions
    vector<Value*> valueArgs;
    for (ExprVector::const_iterator iter = args.begin(); iter < args.end(); 
         ++iter) {
        (*iter)->emit(context);
        valueArgs.push_back(lastValue);
    }
    
            
    IRBuilder<> builder(block);
    lastValue =
        builder.CreateCall(BFuncDefPtr::dcast(func)->rep, valueArgs.begin(),
                           valueArgs.end(),
                           "tmp"
                           );
}

void LLVMBuilder::emitStrConst(model::Context &context, 
                               const StrConstPtr &val) {
    BStrConstPtr bval = BStrConstPtr::dcast(val);
    // if the global string hasn't been defined yet, create it
    if (!bval->rep) {
        IRBuilder<> builder(block);
        bval->rep = builder.CreateGlobalStringPtr(val->val.c_str());
    }
    lastValue = bval->rep;
}

void LLVMBuilder::emitIntConst(model::Context &context, const IntConst &val) {
    lastValue = dynamic_cast<const BIntConst &>(val).rep;
}

Value *emitGEP(IRBuilder<> &builder, Value *obj) {
    Value *zero = llvm::ConstantInt::get(llvm::Type::Int32Ty, 0);
    Value *gepArgs[] = { zero, zero };
    return builder.CreateGEP(obj, zero);
}
    
VarDefPtr LLVMBuilder::emitVarDef(Context &context, const TypeDefPtr &type,
                                  const string &name,
                                  const ExprPtr &initializer
                                  ) {
    // do initializion
    if (initializer)
        initializer->emit(context);
    else
        type->defaultInitializer->emit(context);
    
    // XXX create a local, global, class or instance variable depending on the 
    // context type.
    BTypeDef *tp = dynamic_cast<BTypeDef *>(type.obj);
//    Value *var = new GlobalVariable(tp->rep,
//                                    false, // isConstant
//                                    GlobalValue::InternalLinkage, // linkage tp
//                                    0, // initializer
//                                    name,
//                                    module
//                                    );
    
    // allocate the variable and assign it
    IRBuilder<> builder(block);
    // XXX experimenting with Load
    Value *var = builder.CreateAlloca(tp->rep, 0);
//    Value *tmp = builder.CreateLoad(emitGEP(builder, lastValue));
    Value *tmp = lastValue;
    
    // XXX experimenting with GEP
    // Value *zero = llvm::ConstantInt::get(llvm::Type::Int32Ty, 0);
    // Value *gepArgs[] = { zero, zero };
    // builder.CreateGEP(var, gepArgs, gepArgs + 2 )

    lastValue = builder.CreateStore(tmp, var);
    
    return new BVarDef(type, name, var);
}
 
void LLVMBuilder::emitVarRef(model::Context &context,
                             const model::VarRef &var
                             ) {
    IRBuilder<> builder(block);
    BVarDefPtr def = BVarDefPtr::dcast(var.def);
    lastValue = builder.CreateLoad(def->rep);
}
                                
void LLVMBuilder::createModule(const char *name) {
    assert(!module);
    module = new llvm::Module(name);
    llvm::Constant *c =
        module->getOrInsertFunction("__main__", llvm::IntegerType::get(32), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    block = BasicBlock::Create("__main__", func);
}

void LLVMBuilder::closeModule() {
    assert(module);
    IRBuilder<> builder(block);
    builder.CreateRet(lastValue);
    
    // create the execution engine
    execEng = llvm::ExecutionEngine::create(module);

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

model::StrConstPtr LLVMBuilder::createStrConst(const std::string &val) {
    return new BStrConst(val);
}

IntConstPtr createIntConst(long val) {
    return new BIntConst(val);
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(const string &funcName) {
    return new FuncCall(funcName);
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
    gd->byteptrType->defaultInitializer = createStrConst("");
    context.addDef(gd->byteptrType);
    
    const llvm::Type *llvmInt32Type = llvm::IntegerType::get(32);
    gd->int32Type = new BTypeDef("int32", llvmInt32Type);
    gd->int32Type->defaultInitializer = createIntConst(0);
    context.addDef(gd->int32Type);
    
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
    funcDef->args[0] = new ArgDef("text", gd->byteptrType);
    context.addDef(DefPtr::ucast(funcDef));
}

void LLVMBuilder::kludge(model::Context &context) {
    model::FuncCall::ExprVector args(1);
    args[0] = new StrConst("hello world");
    emitFuncCall(context, context.lookUp("print"), args);
}

void LLVMBuilder::run() {
//    PassManager passMan;
    llvm::verifyModule(*module, llvm::PrintMessageAction);
//    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
//    passMan.run(*module);
    
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}
