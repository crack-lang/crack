// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LLVMLinkerBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "model/Context.h"
#include "BTypeDef.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "Native.h"
#include "Cacher.h"

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

    for (ModuleListType::reverse_iterator i =
             moduleList->rbegin();
         i != moduleList->rend();
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
    string mainModuleName;
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

        // if this is the main module, store its name.
        string moduleName = (*i)->getNamespaceName();
        if (!moduleName.compare(0, 6, ".main."))
            mainModuleName = moduleName;
    }

    // final linked IR
    Module *finalir = linker->getModule();

    // final IR generation: cleanup and main
    emitAggregateCleanup(finalir);
    BTypeDef *vtableType =
        BTypeDefPtr::rcast(context.construct->vtableBaseType);
    Value *vtableTypeBody = vtableType->getClassInstRep(finalir, 0);
    createMain(finalir, options.get(), vtableTypeBody, mainModuleName);

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
    result->intzLLVM = intzLLVM;
    return result;
}

ModuleDefPtr LLVMLinkerBuilder::createModule(Context &context,
                                             const string &name,
                                             const string &path,
                                             ModuleDef *owner
                                             ) {

    assert(!module);
    LLVMContext &lctx = getGlobalContext();
    createLLVMModule(name);

    if (options->debugMode)
        debugInfo = new DebugInfo(module,
                                  name,
                                  path,
                                  context.builder.options.get()
                                  );

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

    // branch to the actual first block of the function
    builder.SetInsertPoint(mainInsert);
    BasicBlock *temp = BasicBlock::Create(lctx, "moduleBody", func);
    builder.CreateBr(temp);
    builder.SetInsertPoint(temp);

    createModuleCommon(context);

    bModDef =  new BModuleDef(name, context.ns.get(), module);
    bModDef->sourcePath = getSourcePath(path);

    return bModDef;
}

void LLVMLinkerBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    LLVMContext &lctx = getGlobalContext();

    // add the module to the list
    addModule(BModuleDefPtr::cast(moduleDef));

    // get the "initialized" flag
    GlobalVariable *moduleInit = module->getNamedGlobal(moduleDef->name +
                                                         ":initialized"
                                                        );

    // if there was a top-level throw, we could already have a terminator.
    // Generate the code to set the init flag and a return instruction if not.
    if (!builder.GetInsertBlock()->getTerminator()) {

        if (moduleDef->fromExtension) {
            // if this is an extension, we create a runtime initialize call
            // this allows the extension to initialize, but also ensures the
            // extension will be linked since ld requires at least one call into it
            // XXX real mangle? see Construct::loadSharedLib
            string name = moduleDef->getFullName();
            for (int i=0; i < name.size(); ++i) {
                if (name[i] == '.')
                    name[i] = '_';
            }
            Constant *initFunc =
                module->getOrInsertFunction(name + "_rinit",
                                            Type::getVoidTy(getGlobalContext()),
                                            NULL
                                            );
            Function *f = llvm::cast<llvm::Function>(initFunc);
            builder.CreateCall(f);
        }

        // at the end of the code for the module, set the "initialized" flag.
        builder.CreateStore(Constant::getIntegerValue(
                                Type::getInt1Ty(lctx),APInt(1,1,false)
                             ),
                            moduleInit
                            );

        builder.CreateRetVoid();
    }

    // since the cleanups have to be emitted against the module context, clear
    // the unwind blocks so we generate them for the del function.
    clearCachedCleanups(context);

    // emit the cleanup function for this module
    // we will emit calls to these (for all modules) during run() in the finalir
    string cleanupFuncName = moduleDef->name + ":cleanup";
    llvm::Constant *c =
        module->getOrInsertFunction(cleanupFuncName,
                                    Type::getVoidTy(lctx), NULL);
    Function *initFunc = func;
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    // create a new exStruct variable for this context in the standard start
    // block.
    createFuncStartBlocks(cleanupFuncName);
    createSpecialVar(context.ns.get(), getExStructType(), ":exStruct");

    // if the initialization flag is not set, branch to return
    BasicBlock *cleanupBlock = BasicBlock::Create(lctx, ":cleanup", func),
               *retBlock = BasicBlock::Create(lctx, "done", func);
    Value *initVal = builder.CreateLoad(moduleInit);
    builder.CreateCondBr(initVal, cleanupBlock, retBlock);

    builder.SetInsertPoint(cleanupBlock);

    closeAllCleanupsStatic(context);
    builder.CreateBr(retBlock);

    builder.SetInsertPoint(retBlock);
    builder.CreateRetVoid();

    // emit a table of address/function for the module
    vector<Constant *> funcVals;
    Type *byteType = builder.getInt8Ty();
    Type *bytePtrType = byteType->getPointerTo();
    Constant *zero = ConstantInt::get(Type::getInt32Ty(lctx), 0);
    Constant *index00[] = { zero, zero };
    Module::FunctionListType &funcList = module->getFunctionList();
    for (Module::FunctionListType::iterator funcIter = funcList.begin();
         funcIter != funcList.end();
         ++funcIter
         ) {
        string name = funcIter->getName();
        if (!funcIter->isDeclaration()) {
            funcVals.push_back(ConstantExpr::getBitCast(funcIter,
                                                        bytePtrType
                                                        )
                               );
            ArrayType *byteArrType =
                ArrayType::get(byteType, name.size() + 1);
            Constant *funcName = ConstantDataArray::getString(lctx, name, true);
            GlobalVariable *nameGVar =
                new GlobalVariable(*module, byteArrType, true, // is constant
                                   GlobalValue::InternalLinkage,
                                   funcName,
                                   moduleDef->name +
                                   ":debug_func_name",
                                   0,
                                   false
                                   );
            Constant *namePtr =
                ConstantExpr::getGetElementPtr(nameGVar, index00, 2);
            funcVals.push_back(namePtr);
        }
    }
    funcVals.push_back(Constant::getNullValue(bytePtrType));
    ArrayType *bytePtrArrType =
        ArrayType::get(bytePtrType, funcVals.size());
    GlobalVariable *funcTable = new GlobalVariable(
        *module, bytePtrArrType,
        true,
        GlobalValue::InternalLinkage,
        ConstantArray::get(bytePtrArrType,
                            funcVals
                            ),
        moduleDef->name + ":debug_func_table",
        0,
        false
    );

    // call the function to populate debug info.
    vector<Type *> argTypes(1);
    argTypes[0] = bytePtrType->getPointerTo();
    FunctionType *funcType = FunctionType::get(builder.getVoidTy(), argTypes,
                                              false
                                              );
    Function *registerFunc =
        cast<Function>(module->getOrInsertFunction("__CrackRegisterFuncTable",
                                                   funcType
                                                   )
                       );
    vector<Value *> args(1);
    args[0] = ConstantExpr::getGetElementPtr(funcTable, index00, 2);
    BasicBlock &entryBlock = initFunc->getEntryBlock();
    builder.SetInsertPoint(&entryBlock, entryBlock.begin());
    builder.CreateCall(registerFunc, args);

    if (context.construct->cacheMode) {
        Cacher c(context, options.get(), BModuleDefPtr::acast(moduleDef));
        c.saveToCache();
    }

    if (debugInfo)
        delete debugInfo;
}

void *LLVMLinkerBuilder::loadSharedLibrary(const string &name) {
    if (rootBuilder) {
        rootBuilder->loadSharedLibrary(name);
    } else {
        sharedLibs.push_back(name);
        LLVMBuilder::loadSharedLibrary(name);
    }
}

void LLVMLinkerBuilder::initializeImport(model::ModuleDef* m,
                                         const ImportedDefVec &symbols,
                                         bool annotation) {

    assert(!annotation && "annotation given to linker builder");

    initializeImportCommon(m, symbols);

    // we add a call into our module's :main function
    // to run the top level function of the imported module
    // each :main is only run once, however, so that a module imported
    // from two different modules will have its top level code only
    // run once. this is handled in the :main function itself.
    BasicBlock *orig = builder.GetInsertBlock();
    assert(mainInsert && "no main insert block");

    // if the last instruction is terminal, we need to insert before it
    // TODO: reuse createFuncStartBlock for this
    BasicBlock::iterator i = mainInsert->end();
    if (i != mainInsert->begin() && !(--i)->isTerminator())
        // otherwise insert after it.
        ++i;
    IRBuilder<> b(mainInsert, i);

    Constant *fc =
        module->getOrInsertFunction(m->name+":main",
                                    Type::getVoidTy(getGlobalContext()),
                                    NULL
                                    );
    Function *f = llvm::cast<llvm::Function>(fc);
    b.CreateCall(f);
}

void LLVMLinkerBuilder::engineFinishModule(model::Context &context,
                                           BModuleDef *moduleDef
                                           ) {
    // only called from registerPrimFuncs in base LLVMBuilder
    addModule(moduleDef);
}

void LLVMLinkerBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(module, 0);
}

model::ModuleDefPtr LLVMLinkerBuilder::materializeModule(
    model::Context &context,
    const std::string &canonicalName,
    model::ModuleDef *owner
) {

    Cacher c(context, options.get());
    return c.maybeLoadFromCache(canonicalName);
}

ExecutionEngine *LLVMLinkerBuilder::getExecEng() const {
    return 0;
}
