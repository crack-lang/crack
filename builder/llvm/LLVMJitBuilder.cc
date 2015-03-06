// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LLVMJitBuilder.h"

#include "model/OverloadDef.h"
#include "model/StatState.h"
#include "BJitModuleDef.h"
#include "DebugInfo.h"
#include "StructResolver.h"
#include "BTypeDef.h"
#include "model/Context.h"
#include "FuncBuilder.h"
#include "Utils.h"
#include "BBuilderContextData.h"
#include "debug/DebugTools.h"
#include "Cacher.h"
#include "spug/check.h"
#include "spug/stlutil.h"
#include "ModuleMerger.h"
#include "Ops.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>
// #include <llvm/Target/TargetData.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>  // link in the JIT
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Debug.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;


LLVMJitBuilder::~LLVMJitBuilder() {
    if (moduleMerger)
        delete moduleMerger;
}

ModuleDefPtr LLVMJitBuilder::registerPrimFuncs(model::Context &context) {

    ModuleDefPtr mod = LLVMBuilder::registerPrimFuncs(context);
    if (!context.construct->cacheMode)
        return mod;

    BModuleDefPtr bMod = BModuleDefPtr::rcast(mod);
}

void LLVMJitBuilder::initialize(model::Context &context) {

    // In the jit, we initialize root builders with the merged module so we
    // have access to everything from annotations.
    ModuleMerger *merger = getModuleMerger();
    module = merger->getTarget();
    moduleDef = instantiateModule(context, ".root", module);
    moduleDef->repId = merger->getRepId();
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
    Function *delFunc = moduleDef->rep->getFunction(moduleDef->name + ":cleanup");
    if (delFunc) {
        void *addr = execEng->getPointerToFunction(delFunc);
        SPUG_CHECK(addr, "Unable to resolve cleanup function");
        moduleDef->cleanup = reinterpret_cast<void (*)()>(addr);
    }
}

void LLVMJitBuilder::fixupAfterMerge(ModuleDef *moduleDef, Module *merged) {
    // Add the module to the list of modules where we need to import the
    // cleanup.
    if (rootBuilder)
        LLVMJitBuilderPtr::rcast(rootBuilder)->needsCleanup.push_back(
            BModuleDefPtr::cast(moduleDef)
        );
    else
        needsCleanup.push_back(BModuleDefPtr::cast(moduleDef));
}

void LLVMJitBuilder::innerFinishModule(Context &context,
                                       BModuleDef *moduleDef) {
   // note, this->module and moduleDef->rep should be ==

    // XXX right now, only checking for > 0, later perhaps we can
    // run specific optimizations at different levels
    if (options->optimizeLevel) {

        // optimize
        llvm::PassManager passMan;

        // Set up the optimizer pipeline.  Start with registering info about how
        // the target lays out data structures.
        passMan.add(new DataLayout(*getExecEng()->getDataLayout()));
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

    // mark the module as finished
    moduleDef->rep->getOrInsertNamedMetadata("crack_finished");
}

void LLVMJitBuilder::engineFinishModule(Context &context,
                                        BModuleDef *moduleDef) {
    innerFinishModule(context, moduleDef);
    mergeModule(moduleDef);
    moduleDef->clearRepFromConstants();
    delete module;
    module = 0;
}

void LLVMJitBuilder::mergeModule(ModuleDef *moduleDef) {
    ModuleMerger *merger = getModuleMerger();
    merger->merge(module);
    BModuleDefPtr::acast(moduleDef)->repId = merger->getRepId();
    BModuleDefPtr::acast(moduleDef)->rep = merger->getTarget();
    fixupAfterMerge(moduleDef, merger->getTarget());
    SPUG_FOR(vector<ModuleDefPtr>, i, moduleDef->getSlaves())
        fixupAfterMerge(BModuleDefPtr::rcast(*i), merger->getTarget());
}

void LLVMJitBuilder::fixClassInstRep(BTypeDef *type) {
    type->getClassInstRep(moduleDef.get());
}

BModuleDef *LLVMJitBuilder::instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *owner
                                              ) {
    return new BJitModuleDef(name, context.ns.get(), owner, getNextModuleId(),
                             0
                             );
}

namespace {
    class LLVMEventListener : public JITEventListener {
        public:
            void NotifyFunctionEmitted(const Function &func,
                                       void *ptr,
                                       size_t size,
                                       const EmittedFunctionDetails &details
                                       ) {
                crack::debug::registerDebugInfo(ptr, func.getName(),
                                                "", // filename
                                                0 // line number
                                                );
            }
    };

    LLVMEventListener eventListener;
}

ExecutionEngine *LLVMJitBuilder::bindJitModule(Module *mod) {
    if (execEng) {
        execEng->addModule(mod);
    } else {
        if (rootBuilder) {
            execEng =
                LLVMJitBuilderPtr::cast(rootBuilder.get())->bindJitModule(mod);
        } else {

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
            execEng->RegisterJITEventListener(&eventListener);
            if (!spug::contains(options->optionMap, "nolazy"))
                execEng->DisableLazyCompilation(false);
        }
    }

    return execEng;
}

ModuleMerger *LLVMJitBuilder::getModuleMerger() {
    if (rootBuilder) {
        return LLVMJitBuilderPtr::rcast(rootBuilder)->getModuleMerger();
    } else if (!moduleMerger) {
        moduleMerger = new ModuleMerger("merged-modules", getNextModuleId(),
                                        getExecEng()
                                        );
        bindJitModule(moduleMerger->getTarget());
    }
    return moduleMerger;
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          Function* real) {
    // In the case of a finished module, we must be able to jit it and get the
    // address.  Jitting happens when we actually run the code, so we
    // wouldn't have to do it here except for the .builtins module, which
    // cannot be jitted until after the runtime module is loaded because it
    // depends on the exception personality function in crack.runtime.
    if (real->getParent()->getNamedMetadata("crack_finished")) {
        void *realAddr = execEng->getPointerToFunction(real);
        SPUG_CHECK(realAddr,
                   "no address for function " << string(real->getName()));
        execEng->updateGlobalMapping(pointer, realAddr);

    // if the module isn't finished, get the pointer if it's available.  If
    // it's not available, then it should be because we are in a cyclic
    // group, and the function will get resolved when the entire group is
    // merged.
    } else if (void *addr = execEng->getPointerToGlobalIfAvailable(real)) {
        execEng->updateGlobalMapping(pointer, addr);
    }
}

void LLVMJitBuilder::addGlobalFuncMapping(Function* pointer,
                                          void* real) {
    getExecEng()->updateGlobalMapping(pointer, real);
}

void LLVMJitBuilder::addGlobalVarMapping(GlobalValue* pointer,
                                         GlobalValue* real) {
    // See comments in addGlobalFuncMapping().
    if (real->getParent()->getNamedMetadata("crack_finished"))
        execEng->updateGlobalMapping(pointer,
                                     execEng->getPointerToGlobal(real)
                                     );
    else if (void *addr = execEng->getPointerToGlobalIfAvailable(real))
        execEng->updateGlobalMapping(pointer, addr);
}

void LLVMJitBuilder::recordShlibSym(const string &name) {
    getShlibSyms().insert(name);
}

void *LLVMJitBuilder::getFuncAddr(llvm::Function *func) {
    void *addr = execEng->getPointerToFunction(func);
    SPUG_CHECK(addr,
               "Unable to resolve function " << string(func->getName()));
    return addr;
}

void LLVMJitBuilder::recordOrphanedDef(VarDef *def) {
    moduleDef->orphanedDefs.push_back(def);
}

void LLVMJitBuilder::run() {
    ExecutionEngine *execEng = getExecEng();
    Module *mainMod = getModuleMerger()->getTarget();
    execEng->addModule(mainMod);
    int (*fptr)() = (int (*)())execEng->getPointerToFunction(
         mainMod->getFunction(func->getName())
    );
    SPUG_CHECK(fptr, "no address for function " << string(func->getName()));
    fptr();
}

FuncDefPtr LLVMJitBuilder::createExternFunc(
    model::Context &context,
    model::FuncDef::Flags flags,
    const std::string &name,
    model::TypeDef *returnType,
    model::TypeDef *receiverType,
    const std::vector<model::ArgDefPtr> &args,
    void *cfunc,
    const char *symbolName
) {
    FuncDefPtr result =
        LLVMBuilder::createExternFunc(context, flags, name, returnType,
                                      receiverType,
                                      args,
                                      cfunc,
                                      symbolName
                                      );
    if (context.construct->cacheMode)
        crack::debug::registerDebugInfo(cfunc, name, "", 0);
    return result;
}

BuilderPtr LLVMJitBuilder::createChildBuilder() {
    return new LLVMJitBuilder(rootBuilder ? rootBuilder.get() : this);
}

ModuleDefPtr LLVMJitBuilder::innerCreateModule(Context &context,
                                               const string &name,
                                               ModuleDef *owner
                                               ) {
    return new BJitModuleDef(name, context.ns.get(), module,
                             getNextModuleId(),
                             BJitModuleDefPtr::cast(owner)
                             );

}

void LLVMJitBuilder::innerCloseModule(Context &context, ModuleDef *moduleDef) {
    finishModule(context, moduleDef);
    if (!moduleDef->isSlave()) { // slave modules aren't complete yet.
// XXX in the future, only verify if we're debugging
//    if (debugInfo)
        verifyModule(*module, llvm::PrintMessageAction);

        // Do the common stuff (common with the .builtin module, which doesn't get
        // closed)
        innerFinishModule(context, BModuleDefPtr::cast(moduleDef));
    }

    // store primitive functions from an extension
    if (moduleDef->fromExtension) {
        for (map<Function *, void *>::iterator iter = primFuncs.begin();
             iter != primFuncs.end();
             ++iter)
            addGlobalFuncMapping(iter->first, iter->second);
    }

    if (debugInfo)
        delete debugInfo;

    // Dump if requested.
    {
        StatState sState(&context, ConstructStats::executor);
        if (options->dumpMode)
            dump();
    }

    // and if we're caching, store it in the persistent cache.
    if (moduleDef->cacheable && context.construct->cacheMode)
        context.cacheModule(moduleDef);

    // Now merge and remove the original module.
    if (!moduleDef->isSlave()) {
        mergeModule(moduleDef);
        delete module;
    } else {
        // This is a slave.  It shares a rep with its master and will be
        // merged with its master, so we can just directly update its rep.
        ModuleMerger *merger = getModuleMerger();
        BModuleDefPtr::acast(moduleDef)->rep = merger->getTarget();
        BModuleDefPtr::acast(moduleDef)->repId = merger->getRepId();
    }
    this->moduleDef->clearRepFromConstants();
    module = 0;
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

void LLVMJitBuilder::registerDef(Context &context, VarDef *varDef) {

    // here we keep track of which external functions and globals came from
    // which module, so we can do a mapping in cacheMode
    if (!context.construct->cacheMode)
        return;

    // get rep from either a BFuncDef or varDef->impl global, then use that as
    // value to the resolver
    BGlobalVarDefImpl *bgbl;
    BFuncDef *fd;
    LLVMBuilder &builder = dynamic_cast<LLVMBuilder &>(context.builder);
    GlobalValue *rep;
    if (varDef->impl && (bgbl = BGlobalVarDefImplPtr::rcast(varDef->impl)))
        // global
        GlobalValue *rep = dyn_cast<GlobalValue>(bgbl->getRep(builder));
    else if (fd = BFuncDefPtr::cast(varDef))
        // funcdef
        GlobalValue *rep = dyn_cast<GlobalValue>(bgbl->getRep(builder));
    else
        //assert(0 && "registerDef: unknown varDef type");
        // this happens in a call from parser (not cacher) on e.g. classes,
        // which we don't care about here
        return;

    SPUG_CHECK(varDef->getFullName() == rep->getName().str(),
               "global def " << varDef->getFullName() <<
                " doees not have the same name as its rep: " <<
                rep->getName().str()
               );

}

void LLVMJitBuilder::registerCleanups() {
    // While we're at it, do setupCleanup() on all of the modules on our list.
    LLVMJitBuilder *b = rootBuilder ? LLVMJitBuilderPtr::rcast(rootBuilder) :
                                      this;
    SPUG_FOR(std::vector<BModuleDefPtr>, i, b->needsCleanup)
        setupCleanup(i->get());
    b->needsCleanup.clear();
}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(
    Context &context,
    CacheFile *cacheFile,
    const string &canonicalName,
    ModuleDef *owner
) {
    if (owner) {
        BJitModuleDefPtr bmod = new BJitModuleDef(canonicalName,
                                                  context.ns.get(),
                                                  module,
                                                  getNextModuleId(),
                                                  0
                                                  );
        owner->addSlave(bmod.get());
        return bmod;
    }

    LLVMCacheFile *cf = LLVMCacheFilePtr::cast(cacheFile);
    BJitModuleDefPtr bmod = cf->maybeLoadFromCache();

    moduleDef = bmod;
    if (bmod) {

        // we materialized a module from bitcode cache
        // find the main function
        module = bmod->rep;

        // convert all of the known types in the module to their existing
        // instances
        StructResolver structResolver(module);
        structResolver.buildTypeMap();
        structResolver.run();

        engineBindModule(bmod.get());
        doRunOrDump(context);

        mergeModule(bmod.get());

        // This is where we would like to delete the source module.
        // Unfortunately, this is problematic after type mutation because
        // constants are cached in the module keyed off of their original
        // type.  Restoring the original type didn't fix it in all cases, and
        // I got tired of debugging it so we currently just leak the module.
        // The original comment was:
        //// Restore the original types prior to destroying the orignal module
        //// so the module constant table (referenced during destruction) is
        //// accurate.
//        structResolver.restoreOriginalTypes();
//        delete module;
//        bmod->clearRepFromConstants();

        // In this case, we set the module to the merged module.
        ModuleMerger *merger = getModuleMerger();
        module = merger->getTarget();
        bmod->repId = merger->getRepId();
    }

    return bmod;
}
