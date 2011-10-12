// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMJitBuilder.h"
#include "BJitModuleDef.h"
#include "DebugInfo.h"
#include "BTypeDef.h"
#include "model/Context.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "debug/DebugTools.h"
#include "Cacher.h"

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
#include <llvm/IntrinsicInst.h>
#include <llvm/Intrinsics.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


void LLVMJitBuilder::engineBindModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==
    // XXX only called from registerPrimFuncs currently
    bindJitModule(moduleDef->rep);
    if (options->dumpMode)
        dump();
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
    
    // mark the module as finished
    moduleDef->rep->getOrInsertNamedMetadata("crack_finished");
}

void LLVMJitBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(module, execEng);
}

BModuleDef *LLVMJitBuilder::instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *owner
                                              ) {
    return new BJitModuleDef(name, context.ns.get(), owner, 0);
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
    // if the module containing the original function has been finished, just 
    // add the global mapping.
    if (real->getParent()->getNamedMetadata("crack_finished")) {
        void *realAddr = execEng->getPointerToFunction(real);
        execEng->addGlobalMapping(pointer, realAddr);
    } else {
        // push this on the list of externals - we used to assign a global mapping 
        // for these right here, but that only works if we're guaranteed that an 
        // imported module is closed before any of its functions are used by the 
        // importer, and that is no longer the case after generics and ephemeral 
        // modules.
        externals.push_back(pair<Function *, Function *>(pointer, real));
    }
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
                                          const string &name,
                                          const string &path,
                                          ModuleDef *owner
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

    bModDef =
        new BJitModuleDef(name, context.ns.get(), module, 
                          owner ? BJitModuleDefPtr::acast(owner) : 0
                          );

    bModDef->path = getSourcePath(path);

    return bModDef;
}

void LLVMJitBuilder::innerCloseModule(Context &context, ModuleDef *moduleDef) {
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

    // work around for an LLVM bug: When doing one of its internal exception 
    // handling passes, LLVM can insert llvm.eh.exception() intrinsics with 
    // calls to an llvm.eh.exception() function that are not part of the 
    // module.  So this loop replaces all such calls with the correct instance 
    // of the function.
    Function *ehEx = getDeclaration(module, Intrinsic::eh_exception);
    for (Module::iterator func = module->begin(); func != module->end();
         ++func
         )
        for (Function::iterator block = func->begin(); block != func->end(); 
             ++block
             )
            for (BasicBlock::iterator inst = block->begin();
                 inst != block->end();
                 ++inst
                 ) {
                IntrinsicInst *intrInst;
                if ((intrInst = dyn_cast<IntrinsicInst>(inst)) &&
                    intrInst->getIntrinsicID() == Intrinsic::eh_exception &&
                    intrInst->getCalledFunction() != ehEx
                    )
                    intrInst->setCalledFunction(ehEx);
            }

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

    // resolve all externals
    for (int i = 0; i < externals.size(); ++i) {
        void *realAddr = execEng->getPointerToFunction(externals[i].second);
        execEng->addGlobalMapping(externals[i].first, realAddr);
    }
    externals.clear();
    
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

    doRunOrDump(context);

}
void LLVMJitBuilder::doRunOrDump(Context &context) {

    // dump or run the module depending on the mode.
    if (rootBuilder->options->statsMode)
        context.construct->stats->switchState(ConstructStats::run);

    if (options->dumpMode)
        dump();

    if (!options->dumpMode || !context.construct->compileTimeConstruct)
        run();

    if (rootBuilder->options->statsMode)
        context.construct->stats->switchState(ConstructStats::build);

}

void LLVMJitBuilder::closeModule(Context &context, ModuleDef *moduleDef) {
    assert(module);
    BJitModuleDefPtr::acast(moduleDef)->closeOrDefer(context, this);    
}

void LLVMJitBuilder::dump() {
    PassManager passMan;
    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
    passMan.run(*module);
}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(model::Context &context,
                                                   const std::string &canonicalName,
                                                   const std::string &path,
                                                   model::ModuleDef *owner) {

    Cacher c(context, options.get());
    BModuleDef *bmod = c.maybeLoadFromCache(canonicalName, path);

    if (bmod) {

        // we materialized a module from bitcode cache
        // find the main function
        module = bmod->rep;

        NamedMDNode *node = module->getNamedMetadata("crack_entry_func");
        assert(node && "no crack_entry_func");
        MDNode *funcNode = node->getOperand(0);
        assert(funcNode && "malformed crack_entry_func");
        func = dyn_cast<Function>(funcNode->getOperand(0));
        assert(func && "entry function not LLVM Function!");

        engineBindModule(bmod);
        doRunOrDump(context);

    }

    return bmod;

}

void LLVMJitBuilder::cacheModule(model::Context &context, model::ModuleDefPtr mod) {

    // encode main function location in bitcode metadata
    vector<Value *> dList;
    NamedMDNode *node;

    node = module->getOrInsertNamedMetadata("crack_entry_func");
    dList.push_back(func);
    node->addOperand(MDNode::get(getGlobalContext(), dList.data(), 1));

    Cacher c(context, options.get(), BModuleDefPtr::rcast(mod));
    c.saveToCache();

}
