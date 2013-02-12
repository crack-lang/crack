// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

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
#include "spug/check.h"

#include <llvm/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT
#include <llvm/Module.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Intrinsics.h>
#include <llvm/Support/Debug.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;

ModuleDefPtr LLVMJitBuilder::registerPrimFuncs(model::Context &context) {

    ModuleDefPtr mod = LLVMBuilder::registerPrimFuncs(context);
    if (!context.construct->cacheMode)
        return mod;

    BModuleDefPtr bMod = BModuleDefPtr::rcast(mod);

    // if we're caching, register .builtin definitions in the cache
    ensureCacheMap();
    for (Module::iterator iter = module->begin();
         iter != module->end();
         ++iter
         ) {
        if (!iter->isDeclaration()) {
            cacheMap->insert(CacheMapType::value_type(iter->getName().str(),
                                                      iter));
        }
    }

    return bMod;

}

ExecutionEngine *LLVMJitBuilder::getExecEng() {
    if (!execEng && rootBuilder)
        execEng = LLVMJitBuilderPtr::arcast(rootBuilder)->getExecEng();
    return execEng;
}

void LLVMJitBuilder::engineBindModule(BModuleDef *moduleDef) {
    // note, this->module and moduleDef->rep should be ==
    bindJitModule(moduleDef->rep);
    if (options->dumpMode)
        dump();
}

void LLVMJitBuilder::setupCleanup(BModuleDef *moduleDef) {
    Function *delFunc = module->getFunction(moduleDef->name + ":cleanup");
    if (delFunc) {
        void *addr = execEng->getPointerToFunction(delFunc);
        SPUG_CHECK(addr, "Unable to resolve cleanup function");
        moduleDef->cleanup = reinterpret_cast<void (*)()>(addr);
    }
}

void LLVMJitBuilder::engineFinishModule(Context &context,
                                        BModuleDef *moduleDef) {

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

    setupCleanup(moduleDef);

    // if we have a cacher, make sure that all globals are registered there.
    if (context.construct->cacheMode) {
        // make sure we have a cache map
        ensureCacheMap();

        Module *module = moduleDef->rep;
        for (Module::global_iterator iter = module->global_begin();
             iter != module->global_end();
             ++iter
             ) {
            if (cacheMap->find(iter->getName()) == cacheMap->end())
                cacheMap->insert(CacheMapType::value_type(iter->getName(),
                                                          iter
                                                          )
                                 );
        }
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

            // we have to specify all of the arguments for this so we can turn
            // off "allocate globals with code."  In addition to being
            // deprecated in the docs for this function, this option causes
            // seg-faults when we allocate globals under certain conditions.
            InitializeNativeTarget();

            EngineBuilder eb(mod);
            eb.setOptLevel(CodeGenOpt::Default).
               setEngineKind(EngineKind::JIT).
               setAllocateGVsWithCode(false);
            TargetMachine *tm = eb.selectTarget();
            tm->Options.JITEmitDebugInfo = true;
            tm->Options.JITExceptionHandling = true;

            execEng = eb.create(tm);

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
        SPUG_CHECK(realAddr,
                   "no address for function " << string(real->getName()));
        execEng->updateGlobalMapping(pointer, realAddr);
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
    execEng->updateGlobalMapping(pointer, real);
}

void LLVMJitBuilder::addGlobalVarMapping(GlobalValue* pointer,
                                         GlobalValue* real) {
    execEng->updateGlobalMapping(pointer, execEng->getPointerToGlobal(real));
}

void *LLVMJitBuilder::getFuncAddr(llvm::Function *func) {
    void *addr = execEng->getPointerToFunction(func);
    SPUG_CHECK(addr,
               "Unable to resolve function " << string(func->getName()));
    return addr;
}

void LLVMJitBuilder::run() {
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(func);
    SPUG_CHECK(fptr, "no address for function " << string(func->getName()));
    fptr();
}

BuilderPtr LLVMJitBuilder::createChildBuilder() {
    LLVMJitBuilder *result = new LLVMJitBuilder();
    result->rootBuilder = rootBuilder ? rootBuilder : this;
    result->llvmVoidPtrType = llvmVoidPtrType;
    result->options = options;
    result->intzLLVM = intzLLVM;
    return result;
}

ModuleDefPtr LLVMJitBuilder::innerCreateModule(Context &context,
                                               const string &name,
                                               ModuleDef *owner
                                               ) {
    bindJitModule(module);
    return new BJitModuleDef(name, context.ns.get(), module,
                             BJitModuleDefPtr::cast(owner)
                             );

}

void LLVMJitBuilder::innerCloseModule(Context &context, ModuleDef *moduleDef) {
    finishModule(context, moduleDef);
// XXX in the future, only verify if we're debugging
//    if (debugInfo)
        verifyModule(*module, llvm::PrintMessageAction);

    // let jit or linker finish module before run/link
    engineFinishModule(context, BModuleDefPtr::cast(moduleDef));

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
        SPUG_CHECK(realAddr,
                   "no address for function " <<
                    string(externals[i].second->getName())
                   );
        execEng->updateGlobalMapping(externals[i].first, realAddr);
    }
    externals.clear();

    // register the globals.
    registerGlobals();

    doRunOrDump(context);

    // and if we're caching, store it in the persistent cache.
    if (moduleDef->cacheable && context.construct->cacheMode)
        context.cacheModule(moduleDef);

}

void LLVMJitBuilder::doRunOrDump(Context &context) {

    // dump or run the module depending on the mode.

    StatState sState(&context, ConstructStats::executor);

    if (options->dumpMode)
        dump();
}

void LLVMJitBuilder::closeModule(Context &context, ModuleDef *moduleDef) {

    assert(module);
    StatState sStats(&context, ConstructStats::builder, moduleDef);
    BJitModuleDefPtr::acast(moduleDef)->closeOrDefer(context, this);

}

void LLVMJitBuilder::dump() {
    PassManager passMan;
    passMan.add(llvm::createPrintModulePass(&llvm::outs()));
    passMan.run(*module);
}

void LLVMJitBuilder::ensureCacheMap() {
    // make sure cacheMap is initialized
    // a single copy of the map exists in the rootBuilder
    if (!cacheMap) {
        if (rootBuilder) {
            LLVMJitBuilder *rb = LLVMJitBuilderPtr::cast(rootBuilder.get());
            if (rb->cacheMap)
                cacheMap = rb->cacheMap;
            else
                cacheMap = rb->cacheMap = new CacheMapType();
        } else {
            // this is the root builder
            cacheMap = new CacheMapType();
        }
    }
}

void LLVMJitBuilder::registerDef(Context &context, VarDef *varDef) {

    // here we keep track of which external functions and globals came from
    // which module, so we can do a mapping in cacheMode
    if (!context.construct->cacheMode)
        return;

    ensureCacheMap();

    // get rep from either a BFuncDef or varDef->impl global, then use that as
    // value to cacheMap
    BGlobalVarDefImpl *bgbl;
    BFuncDef *fd;
    LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
    if (varDef->impl && (bgbl = BGlobalVarDefImplPtr::rcast(varDef->impl))) {
        // global
        cacheMap->insert(
            CacheMapType::value_type(
                varDef->getFullName(),
                dyn_cast<GlobalValue>(bgbl->getRep(builder))
            )
        );
    } else if (fd = BFuncDefPtr::cast(varDef)) {
        // funcdef
        cacheMap->insert(
            CacheMapType::value_type(varDef->getFullName(),
                                     fd->getRep(builder)
                                     )
        );
    } else {
        //assert(0 && "registerDef: unknown varDef type");
        // this happens in a call from parser (not cacher) on e.g. classes,
        // which we don't care about here
        return;
    }

}

void LLVMJitBuilder::registerGlobals() {
    // register debug info for the module
    for (Module::iterator iter = module->begin();
         iter != module->end();
         ++iter
         ) {
        if (!iter->isDeclaration()) {
            string name = iter->getName();
            crack::debug::registerDebugInfo(
                execEng->getPointerToGlobal(iter),
                name,
                "",   // file name
                0     // line number
            );

            // also add the function to the cache.
            if (cacheMap)
                cacheMap->insert(
                    CacheMapType::value_type(iter->getName(), iter)
                );
        }
    }

    // register global variables with the cache while we're at it.
    if (cacheMap) {
        for (Module::global_iterator iter = module->global_begin();
            iter != module->global_end();
            ++iter
            )
            if (!iter->isDeclaration())
                cacheMap->insert(
                    CacheMapType::value_type(iter->getName(), iter)
                );
    }
}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(
    Context &context,
    const string &canonicalName,
    ModuleDef *owner
) {

    Cacher c(context, options.get());
    BModuleDefPtr bmod = c.maybeLoadFromCache(canonicalName);

    if (bmod) {

        // we materialized a module from bitcode cache
        // find the main function
        module = bmod->rep;

        // entry function
        func = c.getEntryFunction();

        engineBindModule(bmod.get());
        ensureCacheMap();

        // try to resolve unresolved globals from the cache
        for (Module::global_iterator iter = module->global_begin();
             iter != module->global_end();
             ++iter
             ) {
            if (iter->isDeclaration()) {

                // now find the defining module
                string xname = iter->getName();
                CacheMapType::const_iterator globalDefIter =
                    cacheMap->find(iter->getName());
                if (globalDefIter != cacheMap->end()) {
                    void *realAddr =
                        execEng->getPointerToGlobal(globalDefIter->second);
                    SPUG_CHECK(realAddr,
                               "unable to resolve global: " <<
                               globalDefIter->first
                               );
                    execEng->updateGlobalMapping(iter, realAddr);
                } else {
                    cout << "couldn't get " << xname << " from cacheMap" <<
                    endl;
                }
            }
        }

        // now try to resolve functions
        for (Module::iterator iter = module->begin();
             iter != module->end();
             ++iter
             ) {
            if (iter->isDeclaration() &&
                !iter->isMaterializable()) {
                // now find the defining module
                CacheMapType::const_iterator globalDefIter =
                    cacheMap->find(iter->getName());
                if (globalDefIter != cacheMap->end()) {
                    Function *f = dyn_cast<Function>(globalDefIter->second);
                    void *realAddr = execEng->getPointerToFunction(f);
                    SPUG_CHECK(realAddr,
                               "Unable to resolve function " <<
                                string(f->getName())
                               );
                    execEng->updateGlobalMapping(iter, realAddr);
                }
            }
        }

        registerGlobals();

        setupCleanup(bmod.get());

        doRunOrDump(context);

    }

    return bmod;

}
