// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMJitBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "BTypeDef.h"
#include "model/Context.h"
#include "FuncBuilder.h"
#include "Utils.h"

#include <llvm/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT
#include <llvm/Module.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


void LLVMJitBuilder::engineBindModule(ModuleDef *moduleDef) {
    bindJitModule(module);
}

void LLVMJitBuilder::engineFinishModule(ModuleDef *moduleDef) {
    // XXX right now, only checking for > 0, later perhaps we can
    // run specific optimizations at different levels
    if (options->optimizeLevel) {
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
    Function *delFunc = module->getFunction("__del__");
    if (delFunc) {
        BModuleDefPtr::cast(moduleDef)->cleanup =
                reinterpret_cast<void (*)()>(
                        execEng->getPointerToFunction(delFunc)
                        );
    }
}

ExecutionEngine *LLVMJitBuilder::bindJitModule(Module *mod) {
    if (execEng) {
        execEng->addModule(mod);
    } else {
        if (rootBuilder)
            execEng = LLVMJitBuilderPtr::cast(rootBuilder.get())->bindJitModule(mod);
        else {

            llvm::JITEmitDebugInfo = true;

            // we have to specify all of the arguments for this so we can turn
            // off "allocate globals with code."  In addition to being
            // deprecated in the docs for this function, this option causes
            // seg-faults when we allocate globals under certain conditions.
            InitializeNativeTarget();
            execEng = ExecutionEngine::create(mod,
                                              false, // force interpreter
                                              0, // error string
                                              CodeGenOpt::Default, // opt lvl
                                              false // alloc globals with code
                                              );
        }
    }

    return execEng;
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          Function* real) {
    execEng->addGlobalMapping(pointer, execEng->getPointerToFunction(real));
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          void* real) {
    execEng->addGlobalMapping(pointer, real);
}

void LLVMJitBuilder::addGlobalVarMapping(GlobalValue* pointer,
                                         GlobalValue* real) {
    execEng->addGlobalMapping(pointer, execEng->getPointerToGlobal(real));
}

void *LLVMJitBuilder::getFuncAddr(llvm::Function *func) {
    return execEng->getPointerToFunction(func);
}

void LLVMJitBuilder::run() {
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    fptr();
}

BuilderPtr LLVMJitBuilder::createChildBuilder() {
    LLVMJitBuilder *result = new LLVMJitBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    result->options = options;
    return result;
}

ModuleDefPtr LLVMJitBuilder::createModule(Context &context,
                                       const string &name
                                       ) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    module = new llvm::Module(name, lctx);

    if (options->debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    llvm::Constant *c =
        module->getOrInsertFunction("__main__", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    builder.SetInsertPoint(BasicBlock::Create(lctx, "__main__", func));

    // name some structs in this module
    BTypeDef *classType = BTypeDefPtr::arcast(context.construct->classType);
    module->addTypeName(".struct.Class", classType->rep);
    BTypeDef *vtableBaseType = BTypeDefPtr::arcast(
                                  context.construct->vtableBaseType);
    module->addTypeName(".struct.vtableBase", vtableBaseType->rep);

    // all of the "extern" primitive functions have to be created in each of
    // the modules - we can not directly reference across modules.

    BTypeDef *int32Type = BTypeDefPtr::arcast(context.construct->int32Type);
    BTypeDef *int64Type = BTypeDefPtr::arcast(context.construct->int64Type);
    BTypeDef *uint64Type = BTypeDefPtr::arcast(context.construct->uint64Type);
    BTypeDef *intType = BTypeDefPtr::arcast(context.construct->intType);
    BTypeDef *voidType = BTypeDefPtr::arcast(context.construct->int32Type);
    BTypeDef *float32Type = BTypeDefPtr::arcast(context.construct->float32Type);
    BTypeDef *byteptrType =
        BTypeDefPtr::arcast(context.construct->byteptrType);
    BTypeDef *voidptrType =
        BTypeDefPtr::arcast(context.construct->voidptrType);

    // create "void *calloc(uint size)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidptrType, "calloc", 2);
        f.addArg("size", intType);
        f.addArg("size", intType);
        f.setSymbolName("calloc");
        f.finish();
        callocFunc = f.funcDef->getRep(*this);
    }

    // create "array[byteptr] __getArgv()"
    {
        TypeDefPtr array = context.ns->lookUp("array");
        assert(array.get() && "array not defined in context");
        TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
        types->push_back(context.construct->byteptrType.get());
        TypeDefPtr arrayOfByteptr =
            array->getSpecialization(context, types.get());
        FuncBuilder f(context, FuncDef::noFlags,
                      BTypeDefPtr::arcast(arrayOfByteptr),
                      "__getArgv",
                      0
                      );
        f.setSymbolName("__getArgv");
        f.finish();
    }

    // create "int __getArgc()"
    {
        FuncBuilder f(context, FuncDef::noFlags, intType, "__getArgc", 0);
        f.setSymbolName("__getArgc");
        f.finish();
    }

    // possibly bind to execution engine
    BModuleDef *moduleDef = new BModuleDef(name, context.ns.get());
    engineBindModule(moduleDef);

    return moduleDef;
}

void LLVMJitBuilder::closeModule(Context &context, ModuleDef *moduleDef) {
    assert(module);
    builder.CreateRetVoid();

    // emit the cleanup function
    Function *mainFunc = func;
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction("__del__", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    builder.SetInsertPoint(BasicBlock::Create(lctx, "__del__", func));
    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();

    // restore the main function
    func = mainFunc;

// XXX in the future, only verify if we're debugging
//    if (debugInfo)
        verifyModule(*module, llvm::PrintMessageAction);

    // let jit or linker finish module before run/link
    engineFinishModule(moduleDef);

    // store primitive functions from an extension
    if (moduleDef->fromExtension) {
        for (map<Function *, void *>::iterator iter = primFuncs.begin();
        iter != primFuncs.end();
        ++iter
                )
            addGlobalFuncMapping(iter->first, iter->second);
    }

    if (debugInfo)
        delete debugInfo;

    // dump or run the module depending on the mode.
    if (options->dumpMode)
        dump();
    else
        run();
}

void LLVMJitBuilder::dump() {
    PassManager passMan;
    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
    passMan.run(*module);
}
