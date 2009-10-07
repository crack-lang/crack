
#include "LLVMBuilder.h"

// LLVM includes
#include <stddef.h>
#include <stdlib.h>
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/PassManager.h"
#include "llvm/CallingConv.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"

#include <model/ArgDef.h>
#include <model/Context.h>
#include <model/FuncDef.h>
#include <model/FuncCall.h>
#include <model/StrConst.h>
#include <model/TypeDef.h>


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
            llvm::Type *rep;
            BTypeDef(const char *name, llvm::Type *rep) :
                model::TypeDef(name),
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
}    

model::StrConstPtr LLVMBuilder::createStrConst(const std::string &val) {
    return new BStrConst(val);
}
                       
model::FuncCallPtr LLVMBuilder::createFuncCall(const string &funcName) {
    return new FuncCall(funcName);
}
    
model::FuncDefPtr LLVMBuilder::createFuncDef(const char *name) {
    return new BFuncDef(name, 0);
}

void LLVMBuilder::registerPrimFuncs(model::Context &context) {
    
    Context::GlobalData *gd = context.globalData;
    // create the basic types
    llvm::Type *llvmBytePtrType = 
        llvm::PointerType::getUnqual(llvm::IntegerType::get(8));
    gd->byteptrType = new BTypeDef("byteptr", llvmBytePtrType);
    context.addDef(gd->byteptrType);
    
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
    
    llvm::ExecutionEngine *execEng = llvm::ExecutionEngine::create(module);
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}



