// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMLinkerBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "model/Context.h"
#include "BTypeDef.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "Native.h"

#include <llvm/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Module.h>
#include <llvm/Linker.h>
#include <llvm/Analysis/Verifier.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


// emit the final cleanup function, a collection of calls
// to the cleanup functions for the individual modules we have
// included in this build
// by convention, the name is "main:cleanup". this is used by Native.cc
Function *LLVMLinkerBuilder::emitAggregateCleanup(Module *module) {

    assert(!rootBuilder && "emitAggregateCleanup must be called from "
                           "root builder");

    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
            module->getOrInsertFunction("main:cleanup",
                                        Type::getVoidTy(lctx), NULL);
    Function *func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    BasicBlock *block = BasicBlock::Create(lctx, "", func);

    for (ModuleListType::const_iterator i =
             moduleList->begin();
         i != moduleList->end();
         ++i) {
         Function *dfunc = module->getFunction((*i)->name+":cleanup");
         // missing a cleanup function isn't an error, because the moduleDef
         // list currently includes modules that don't have any associated
         // codegen, like "crack"
         if (!dfunc)
             continue;
         CallInst::Create(dfunc, "", block);
    }
    ReturnInst::Create(lctx, block);
    return func;
}

// maintain a single list of modules throughout the compile in the root builder
LLVMLinkerBuilder::ModuleListType
  *LLVMLinkerBuilder::addModule(BModuleDef *mod) {

    if (moduleList) {
        moduleList->push_back(mod);
    } else {
        if (rootBuilder)
           moduleList = LLVMLinkerBuilderPtr::cast(
                   rootBuilder.get())->addModule(mod);
        else {
            moduleList = new ModuleListType();
            moduleList->push_back(mod);
        }
    }

    return moduleList;
}


void *LLVMLinkerBuilder::getFuncAddr(llvm::Function *func) {
    assert(false && "LLVMLinkerBuilder::getFuncAddr called");
}

void LLVMLinkerBuilder::finishBuild(Context &context) {

    assert(!rootBuilder && "run must be called from root builder");

    // if optimizing, do module level unit at a time
    if (options->optimizeLevel) {
        for (ModuleListType::iterator i = moduleList->begin();
             i != moduleList->end();
             ++i) {
              if (options->verbosity > 2)
               std::cerr << "optimizing " << (*i)->rep->getModuleIdentifier() <<
                             std::endl;
               optimizeUnit((*i)->rep, options->optimizeLevel);
        }
    }

    // now link all moduloes
    linker = new Linker("crack",
                        "main-module",
                        getGlobalContext(),
                        (options->verbosity > 2) ?
                        Linker::Verbose :
                        0
                        );
    assert(linker && "unable to create Linker");

    string errMsg;
    for (ModuleListType::iterator i = moduleList->begin();
         i != moduleList->end();
         ++i) {
        if (options->verbosity > 2)
            std::cerr << "linking " << (*i)->rep->getModuleIdentifier() <<
                    std::endl;
        linker->LinkInModule((*i)->rep, &errMsg);
        if (errMsg.length()) {
            std::cerr << "error linking " << (*i)->rep->getModuleIdentifier() <<
                    " [" + errMsg + "]\n";
            (*i)->rep->dump();
        }
    }

    // final linked IR
    Module *finalir = linker->getModule();

    // final IR generation: cleanup and main
    emitAggregateCleanup(finalir);
    createMain(finalir, options.get());

    // possible LTO optimizations
    if (options->optimizeLevel) {
        if (options->verbosity > 2)
            std::cerr << "link time optimize final IR" << std::endl;
        optimizeLink(finalir, options->debugMode);
    }

    // if we're not optimizing but we're doing debug, verify now
    // if we are optimizing and we're doing debug, verify is done in the
    // optimization passes instead
    if (!options->optimizeLevel && options->debugMode)
        verifyModule(*module, llvm::PrintMessageAction);

    // if we're dumping, return now that we've added main and finalized ir
    if (options->dumpMode) {
        PassManager passMan;
        passMan.add(llvm::createPrintModulePass(&llvm::outs()));
        passMan.run(*finalir);
        return;
    }

    // finish native compile and link
    nativeCompile(finalir,
                  options.get(),
                  sharedLibs,
                  context.construct->sourceLibPath
                  );

}

BuilderPtr LLVMLinkerBuilder::createChildBuilder() {
    LLVMLinkerBuilder *result = new LLVMLinkerBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    result->options = options;
    return result;
}

ModuleDefPtr LLVMLinkerBuilder::createModule(Context &context,
                                       const string &name) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    createLLVMModule(name);

    if (options->debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    BBuilderContextData::get(&context);

    string funcName = name + ":main";
    llvm::Constant *c =
        module->getOrInsertFunction(funcName, Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    createFuncStartBlocks(funcName);

    // insert point is now at the begining of the :main function for
    // this module. this will run the top level code for the module
    // however, we consult a module level global to ensure that we only
    // run the top level code once, since this module may be imported from
    // more than one place
    GlobalVariable *moduleInit =
            new GlobalVariable(*module,
                               Type::getInt1Ty(lctx),
                               false, // constant
                               GlobalValue::InternalLinkage,
                               Constant::getIntegerValue(
                                       Type::getInt1Ty(lctx),APInt(1,0,false)),
                               name+":initialized"
                               );

    // emit code that checks the global and returns immediately if it
    // has been set to 1
    BasicBlock *alreadyInitBlock = BasicBlock::Create(lctx, "alreadyInit", func);
    assert(!mainInsert);
    mainInsert = BasicBlock::Create(lctx, "topLevel", func);
    Value* currentInitVal = builder.CreateLoad(moduleInit);
    builder.CreateCondBr(currentInitVal, alreadyInitBlock, mainInsert);

    // already init, return
    builder.SetInsertPoint(alreadyInitBlock);
    builder.CreateRetVoid();

    // do init, set global to 1, continue top level..
    builder.SetInsertPoint(mainInsert);
    builder.CreateStore(Constant::getIntegerValue(
                          Type::getInt1Ty(lctx),APInt(1,1,false)),
                        moduleInit);


    createModuleCommon(context);

    // add to module list
    BModuleDef *newModule = new BModuleDef(name, context.ns.get(), module);
    addModule(newModule);

    return newModule;
}

void LLVMLinkerBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    
    // if there was a top-level throw, we could already have a terminator.  
    // Generate a return instruction if not.
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateRetVoid();

    // since the cleanups have to be emitted against the module context, clear 
    // the unwind blocks so we generate them for the del function.
    clearCachedCleanups(context);

    // emit the cleanup function for this module
    // we will emit calls to these (for all modules) during run() in the finalir
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction(moduleDef->name+":cleanup",
                                    Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);
    builder.SetInsertPoint(BasicBlock::Create(lctx, "", func));
    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();

    if (debugInfo)
        delete debugInfo;

}

void *LLVMLinkerBuilder::loadSharedLibrary(const string &name) {
    sharedLibs.push_back(name);
    LLVMBuilder::loadSharedLibrary(name);
}

void LLVMLinkerBuilder::initializeImport(model::ModuleDefPtr m,
                                         bool annotation) {


    // if the module came from an extension, there's no top level to run
    if (m->fromExtension || annotation)
        return;

    // we add a call into our module's :main function
    // to run the top level function of the imported module
    // each :main is only run once, however, so that a module imported
    // from two different modules will have its top level code only
    // run once. this is handled in the :main function itself.
    BasicBlock *orig = builder.GetInsertBlock();
    assert(mainInsert && "no main insert block");
    builder.SetInsertPoint(mainInsert);

    // declaration
    Constant *fc = module->getOrInsertFunction(m->name+":main",
                                              Type::getVoidTy(getGlobalContext()),
                                              NULL);
    Function *f = llvm::cast<llvm::Function>(fc);
    vector<Value*> args;
    builder.CreateCall(f, args.begin(), args.end());

    builder.SetInsertPoint(orig);

}

void LLVMLinkerBuilder::engineFinishModule(BModuleDef *moduleDef) {
    // only called from registerPrimFuncs in base LLVMBuilder
    addModule(moduleDef);
}

void LLVMLinkerBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(module, 0);
}