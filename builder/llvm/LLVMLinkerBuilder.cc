// Copyright 2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "LLVMLinkerBuilder.h"
#include "BModuleDef.h"
#include "DebugInfo.h"
#include "model/Context.h"
#include "BTypeDef.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "Native.h"

#include <llvm/Support/StandardPasses.h>
#include <llvm/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/PassManager.h>
#include <llvm/Module.h>
#include <llvm/Linker.h>

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

    for (vector<ModuleDef *>::const_iterator i =
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

Linker *LLVMLinkerBuilder::linkModule(Module *mod) {
    if (linker) {
        string errMsg;
        linker->LinkInModule(mod, &errMsg);
        if (errMsg.length()) {
            std::cerr << "error linking " << mod->getModuleIdentifier() <<
                    " [" + errMsg + "]\n";
            mod->dump();
        }
    } else {
        if (rootBuilder) {
            if (options->verbosity > 2)
                std::cerr << "linking " << mod->getModuleIdentifier() <<
                        std::endl;
            linker = LLVMLinkerBuilderPtr::cast(
                    rootBuilder.get())->linkModule(mod);
        }
        else {
            linker = new Linker("crack",
                                "main-module",
                                getGlobalContext(),
                                (options->verbosity > 2) ?
                                    Linker::Verbose :
                                    0
                                );
            assert(linker && "unable to create Linker");
            if (options->verbosity > 2)
                std::cerr << "linking " << mod->getModuleIdentifier() <<
                        std::endl;
            linkModule(mod);
        }
    }

    return linker;
}

// maintain a single list of modules throughout the compile in the root builder
LLVMLinkerBuilder::ModuleListType
    *LLVMLinkerBuilder::addModule(ModuleDef *mod) {

    if (moduleList) {
        moduleList->push_back(mod);
    } else {
        if (rootBuilder)
           moduleList = LLVMLinkerBuilderPtr::cast(
                   rootBuilder.get())->addModule(mod);
        else {
            moduleList = new ModuleListType();
        }
    }

    return moduleList;
}


void *LLVMLinkerBuilder::getFuncAddr(llvm::Function *func) {
    assert(false && "LLVMLinkerBuilder::getFuncAddr called");
}

void LLVMLinkerBuilder::finish(Context &context) {

    assert(!rootBuilder && "run must be called from root builder");

    // final linked IR
    Module *finalir = linker->getModule();

    // LTO optimizations
    if (options->optimizeLevel) {
        PassManager passMan;
        createStandardLTOPasses(&passMan,
                                true, // internalize
                                true, // inline
                                options->debugMode // verify each
                                );
        passMan.run(*finalir);
    }

    emitAggregateCleanup(finalir);

    // if we're not optimizing but we're doing debug, verify now
    // if we are optimizing and we're doing debug, verify is done in the
    // LTO passes above instead
    if (!options->optimizeLevel && options->debugMode)
        verifyModule(*finalir, llvm::PrintMessageAction);

    nativeCompile(finalir,
                  options.get(),
                  sharedLibs,
                  context.construct->sourceLibPath
                  );

    if (options->dumpMode) {
        PassManager passMan;
        passMan.add(llvm::createPrintModulePass(&llvm::outs()));
        passMan.run(*finalir);
        return;
    }

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
    module = new llvm::Module(name, lctx);

    if (options->debugMode) {
        debugInfo = new DebugInfo(module, name);
    }

    llvm::Constant *c =
        module->getOrInsertFunction(name+":main", Type::getVoidTy(lctx), NULL);
    func = llvm::cast<llvm::Function>(c);
    func->setCallingConv(llvm::CallingConv::C);

    builder.SetInsertPoint(BasicBlock::Create(lctx, "initCheck", func));

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

    // create "int puts(String)"
    {
        FuncBuilder f(context, FuncDef::noFlags, int32Type, "puts",
                      1
                      );
        f.addArg("text", byteptrType);
        f.setSymbolName("puts");
        f.finish();
    }

    // create "void printint(int32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint", 1);
        f.addArg("val", int32Type);
        f.setSymbolName("printint");
        f.finish();
    }

    // create "void printint64(int64)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printint64", 1);
        f.addArg("val", int64Type);
        f.setSymbolName("printint64");
        f.finish();
    }

    // create "void printuint64(int64)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printuint64", 1);
        f.addArg("val", uint64Type);
        f.setSymbolName("printuint64");
        f.finish();
    }

    // create "void printfloat(float32)"
    {
        FuncBuilder f(context, FuncDef::noFlags, voidType, "printfloat", 1);
        f.addArg("val", float32Type);
        f.setSymbolName("printfloat");
        f.finish();
    }

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

    // add to module list
    ModuleDef *newModule = new BModuleDef(name, context.ns.get());
    addModule(newModule);

    return newModule;
}

void LLVMLinkerBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    builder.CreateRetVoid();

    // emit the cleanup function for this module
    // we will emit calls to these (for all modules) during run() in the finalir
    LLVMContext &lctx = getGlobalContext();
    llvm::Constant *c =
        module->getOrInsertFunction(moduleDef->name+":cleanup",
                                    Type::getVoidTy(lctx), NULL);
    Function *dfunc = llvm::cast<llvm::Function>(c);
    dfunc->setCallingConv(llvm::CallingConv::C);
    builder.SetInsertPoint(BasicBlock::Create(lctx, "", dfunc));
    closeAllCleanupsStatic(context);
    builder.CreateRetVoid();

    linkModule(module);

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

void LLVMLinkerBuilder::engineFinishModule(ModuleDef *moduleDef) {
    // XXX only called for builtin, refactor this
    linkModule(module);
}
