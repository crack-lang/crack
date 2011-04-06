// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMJitBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "BTypeDef.h"
#include "model/Context.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "debug/DebugTools.h"

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
#include <llvm/Intrinsics.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


void LLVMJitBuilder::engineBindModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==
    bindJitModule(moduleDef->rep);
}

void LLVMJitBuilder::engineFinishModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==

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

        passMan.run(*moduleDef->rep);
    }
    Function *delFunc = module->getFunction("__del__");
    if (delFunc) {
        moduleDef->cleanup = reinterpret_cast<void (*)()>(
                                execEng->getPointerToFunction(delFunc)
                             );
    }
}

void LLVMJitBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(module, execEng);
}

ExecutionEngine *LLVMJitBuilder::bindJitModule(Module *mod) {
    if (execEng) {
        execEng->addModule(mod);
    } else {
        if (rootBuilder)
            execEng = LLVMJitBuilderPtr::cast(rootBuilder.get())->bindJitModule(mod);
        else {

            llvm::JITEmitDebugInfo = true;
            llvm::JITExceptionHandling = true;

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
    createLLVMModule(name);

    if (options->debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    // create a context data object
    BBuilderContextData::get(&context);

    llvm::Constant *c =
        module->getOrInsertFunction("__main__", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    createFuncStartBlocks("__main__");

    createModuleCommon(context);
    
    bindJitModule(module);

    BModuleDef *moduleDef = new BModuleDef(name, context.ns.get(), module);
    return moduleDef;
}

void LLVMJitBuilder::closeModule(Context &context, ModuleDef *moduleDef) {
    assert(module);

    // if there was a top-level throw, we could already have a terminator.  
    // Generate a return instruction if not.
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateRetVoid();

    // emit the cleanup function

    // since the cleanups have to be emitted against the module context, clear 
    // the unwind blocks so we generate them for the del function.
    clearCachedCleanups(context);
    
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
    engineFinishModule(BModuleDefPtr::cast(moduleDef));

    // store primitive functions from an extension
    if (moduleDef->fromExtension) {
        for (map<Function *, void *>::iterator iter = primFuncs.begin();
             iter != primFuncs.end();
             ++iter)
            addGlobalFuncMapping(iter->first, iter->second);
    }

    if (debugInfo)
        delete debugInfo;
    
    // build the debug tables
    Module::FunctionListType &funcList = module->getFunctionList();
    for (Module::FunctionListType::iterator funcIter = funcList.begin();
         funcIter != funcList.end();
         ++funcIter
         ) {
        string name = funcIter->getName();
        if (!funcIter->isDeclaration())
            crack::debug::registerDebugInfo(
                execEng->getPointerToGlobal(funcIter),
                name,
                "",   // file name
                0     // line number
            );
    }

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
