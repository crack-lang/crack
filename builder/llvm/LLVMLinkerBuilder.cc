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

#include <llvm/IR/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/IR/Module.h>
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
    linker = new Linker(new Module("main-module", getGlobalContext()));
    assert(linker && "unable to create Linker");

    string errMsg;
    string mainModuleName;
    for (ModuleListType::iterator i = moduleList->begin();
         i != moduleList->end();
         ++i) {
        if ((*i)->isSlave())
            continue;
        if (options->verbosity > 2)
            std::cerr << "linking " << (*i)->rep->getModuleIdentifier() <<
                    std::endl;
        linker->linkInModule((*i)->rep, &errMsg);
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
    Value *vtableTypeBody =
        finalir->getGlobalVariable(vtableType->getFullName() + ":body");
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
    return new LLVMLinkerBuilder(rootBuilder ? rootBuilder.get() : this);
}

ModuleDefPtr LLVMLinkerBuilder::innerCreateModule(Context &context,
                                                  const string &name,
                                                  ModuleDef *owner
                                                  ) {
    return new BModuleDef(name, context.ns.get(), module, getNextModuleId());
}

void LLVMLinkerBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    LLVMContext &lctx = getGlobalContext();

    // add the module to the list
    addModule(BModuleDefPtr::cast(moduleDef));

    finishModule(context, moduleDef);

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
                                   ":debug_func_name"
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
        moduleDef->name + ":debug_func_table"
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
    BasicBlock &entryBlock = func->getEntryBlock();
    builder.SetInsertPoint(&entryBlock, entryBlock.begin());
    builder.CreateCall(registerFunc, args);

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

void LLVMLinkerBuilder::engineFinishModule(model::Context &context,
                                           BModuleDef *moduleDef
                                           ) {
    // only called from registerPrimFuncs in base LLVMBuilder
    addModule(moduleDef);
}

void LLVMLinkerBuilder::fixClassInstRep(BTypeDef *type) {
    // Not sure we ever need to do this since classInst is no longer public.
    type->getClassInstRep(moduleDef.get());
}

model::ModuleDefPtr LLVMLinkerBuilder::materializeModule(
    model::Context &context,
    CacheFile *cacheFile,
    const std::string &canonicalName,
    model::ModuleDef *owner
) {
    return LLVMCacheFilePtr::cast(cacheFile)->maybeLoadFromCache();
}

ExecutionEngine *LLVMLinkerBuilder::getExecEng() {
    return 0;
}
