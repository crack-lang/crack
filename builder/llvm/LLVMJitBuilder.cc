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

bool LLVMJitBuilder::Resolver::trace = true;

void LLVMJitBuilder::Resolver::registerGlobal(ExecutionEngine *execEng,
                                              GlobalValue *globalVal
                                              ) {
    const string &name = globalVal->getName().str();
    pair<CacheMap::iterator, bool> result =
        cacheMap.insert(CacheMap::value_type(name, globalVal));
    if (!result.second)
        // it was already in the cache
        return;

    // back-fill any fixups that were registered.
    FixupMap::iterator i = fixupMap.find(name);
    if (i != fixupMap.end()) {
        void *realAddr = execEng->getPointerToGlobal(globalVal);
        SPUG_CHECK(realAddr,
                   "no address for global " <<
                    string(globalVal->getName())
                   );

        for (GlobalValueVec::iterator j = i->second.begin();
             j != i->second.end();
             ++j
             )
            execEng->updateGlobalMapping(*j, realAddr);

        fixupMap.erase(i);
    }
}

bool LLVMJitBuilder::Resolver::resolve(ExecutionEngine *execEng,
                                       GlobalValue *globalVal
                                       ) {
    const string &name = globalVal->getName().str();

    // first check the cache map for it.
    CacheMap::iterator i = cacheMap.find(name);
    if (i != cacheMap.end()) {
        void *realAddr = execEng->getPointerToGlobal(i->second);
        SPUG_CHECK(realAddr,
                   "no address for global " <<
                    string(globalVal->getName())
                   );

        execEng->updateGlobalMapping(globalVal, realAddr);
        return true;
    }

    // see if it's deferred
    CacheMap::iterator di = deferred.find(name);
    if (di != deferred.end()) {
        // Replace the global with the deferred symbol.  This lets LLVM take
        // care of resolving it for us.  This isn't really kosher, as it adds
        // a global to a module that doesn't own it, but it seems to work.
        globalVal->replaceAllUsesWith(di->second);

        // the module is part of a depencency cycle - don't start collecting
        // addresses in it yet.
        if (trace)
            cerr << "uninstantiated " << globalVal->getName().str() << endl;
        return false;
    }

    // the symbol hasn't been discovered yet, add it to the fixups
    if (trace)
        cerr << "undiscovered " << globalVal->getName().str() << endl;
    fixupMap[name].push_back(globalVal);
    return false;
}

void LLVMJitBuilder::Resolver::deferGlobal(GlobalValue *globalVal) {
    if (!globalVal->isDeclaration()) {
        const string &name = globalVal->getName().str();
        SPUG_CHECK(deferred.insert(make_pair(name, globalVal)).second,
                   name << " already deferred!"
                   );

        // if the symbol exists in the fixup map, replace it in all of the
        // modules that depend on it and remove it from the fixup map.
        FixupMap::iterator i = fixupMap.find(name);
        if (i != fixupMap.end()) {
            if (trace)
                cerr << "fixing up " << name << endl;
            for (GlobalValueVec::iterator j = i->second.begin();
                 j != i->second.end();
                 ++j
                 )
                (*j)->replaceAllUsesWith(globalVal);
            fixupMap.erase(i);
        }
    }
}

void LLVMJitBuilder::Resolver::defer(ExecutionEngine *execEng, Module *module) {
    for (Module::iterator i = module->begin(); i != module->end(); ++i) {
        if (trace)
            cerr << "deferring func " << i->getName().str() << endl;
        deferGlobal(i);
    }

    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i
         ) {
        if (trace)
            cerr << "deferring gvar " << i->getName().str() << endl;
        deferGlobal(i);
    }

    // if we've emptied the fixup map, everything is now in place and we can
    // resolve all of the symbols.
    if (fixupMap.empty()) {
        for (CacheMap::iterator i = deferred.begin(); i != deferred.end();
             ++i
             ) {
            const string &name = i->second->getName();
            void *ptr = execEng->getPointerToGlobal(i->second);
            if (trace)
                cerr << "resolving deferred " << name << "@" << ptr << endl;
            if (dyn_cast<Function>(i->second))
                crack::debug::registerDebugInfo(ptr, name,
                                                "",   // file name
                                                0     // line number
                                                );

            // also add the function to the cache.
            cacheMap.insert(make_pair(name, i->second));
        }

        deferred.clear();
    }
}

LLVMJitBuilder::~LLVMJitBuilder() {
    // clean up the resolver if this is the root builder.
    if (resolver && !rootBuilder)
        delete resolver;
}

void LLVMJitBuilder::Resolver::checkForUnresolvedExternals() {
    if (fixupMap.size()) {
        cerr << "Unresolved externals:" << endl;
        for (FixupMap::iterator iter = fixupMap.begin();
             iter != fixupMap.end();
             ++iter
             )
            cerr << "  " << iter->first << endl;
        SPUG_CHECK(false,
                   "Crack discovered unresolved externals.  This is "
                    "a bug in the executor.  Please report it!"
                   );
    }
}

ModuleDefPtr LLVMJitBuilder::registerPrimFuncs(model::Context &context) {

    ModuleDefPtr mod = LLVMBuilder::registerPrimFuncs(context);
    if (!context.construct->cacheMode)
        return mod;

    BModuleDefPtr bMod = BModuleDefPtr::rcast(mod);

    // if we're caching, register .builtin definitions in the cache
    ensureResolver();
    for (Module::iterator iter = module->begin();
         iter != module->end();
         ++iter
         ) {
        if (!iter->isDeclaration())
            resolver->registerGlobal(execEng, iter);
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
        ensureResolver();

        Module *module = moduleDef->rep;
        for (Module::global_iterator iter = module->global_begin();
             iter != module->global_end();
             ++iter
             )
            resolver->registerGlobal(execEng, iter);
    }

    // mark the module as finished
    moduleDef->rep->getOrInsertNamedMetadata("crack_finished");
}

void LLVMJitBuilder::registerHiddenFunc(model::Context &context,
                                        BFuncDef *func
                                        ) {
    if (context.construct->cacheMode) {
        ensureResolver();
        // we don't currently register debug info for these.
        resolver->registerGlobal(execEng, func->getRep(*this));
    }
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

void LLVMJitBuilder::checkForUnresolvedExternals() {
    if (resolver) resolver->checkForUnresolvedExternals();
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
    if (context.construct->cacheMode) {
        ensureResolver();
        crack::debug::registerDebugInfo(cfunc, name, "", 0);
        resolver->registerGlobal(execEng,
                                 BFuncDefPtr::rcast(result)->getRep(*this)
                                 );
    }
    return result;
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

void LLVMJitBuilder::ensureResolver() {
    // make sure resolver is initialized
    // a single copy of the map exists in the rootBuilder
    if (!resolver) {
        if (rootBuilder) {
            LLVMJitBuilder *rb = LLVMJitBuilderPtr::cast(rootBuilder.get());
            if (rb->resolver)
                resolver = rb->resolver;
            else
                resolver = rb->resolver = new Resolver();
        } else {
            // this is the root builder
            resolver = new Resolver();
        }
    }
}

void LLVMJitBuilder::registerDef(Context &context, VarDef *varDef) {

    // here we keep track of which external functions and globals came from
    // which module, so we can do a mapping in cacheMode
    if (!context.construct->cacheMode)
        return;

    ensureResolver();

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

    resolver->registerGlobal(execEng, rep);
    SPUG_CHECK(varDef->getFullName() == rep->getName().str(),
               "global def " << varDef->getFullName() <<
                " doees not have the same name as its rep: " <<
                rep->getName().str()
               );

}

void LLVMJitBuilder::registerGlobals() {
    // register debug info for the module
    for (Module::iterator iter = module->begin();
         iter != module->end();
         ++iter
         ) {
        if (!iter->isDeclaration()) {
            string name = iter->getName();
            void *ptr = execEng->getPointerToGlobal(iter);
//            cerr << "global " << name << "@" << ptr << endl;
            crack::debug::registerDebugInfo(
                ptr,
                name,
                "",   // file name
                0     // line number
            );

            // also add the function to the cache.
            if (resolver)
                resolver->registerGlobal(execEng, iter);
        }
    }

    // register global variables with the cache while we're at it.
    if (resolver) {
        for (Module::global_iterator iter = module->global_begin();
            iter != module->global_end();
            ++iter
            )
            if (!iter->isDeclaration())
                resolver->registerGlobal(execEng, iter);
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
        ensureResolver();

        // register "__CrackExceptionPersonality" and "_Unwind_Resume"
        if (context.construct->cacheMode) {

            LLVMContext &lctx = getGlobalContext();
            resolver->registerGlobal(execEng, getExceptionPersonalityFunc());
            resolver->registerGlobal(execEng, getUnwindResumeFunc());
        }

        // try to resolve unresolved globals from the cache
        bool hasUnresolvedExternals = false;
        for (Module::global_iterator iter = module->global_begin();
             iter != module->global_end();
             ++iter
             ) {
            if (iter->isDeclaration()) {
                if (!resolver->resolve(execEng, iter))
                    hasUnresolvedExternals = true;
            }
        }

        // now try to resolve functions
        for (Module::iterator iter = module->begin();
             iter != module->end();
             ++iter
             ) {
            if (iter->isDeclaration() && !iter->isMaterializable())
                if (!resolver->resolve(execEng, iter))
                    hasUnresolvedExternals = true;
        }

        if (hasUnresolvedExternals)
            resolver->defer(execEng, module);
        else
            registerGlobals();

        setupCleanup(bmod.get());

        doRunOrDump(context);

    }

    return bmod;

}
