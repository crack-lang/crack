// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LLVMJitBuilder.h"

#include "model/OverloadDef.h"
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
#include <llvm/IR/Module.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/Debug.h>

using namespace std;
using namespace llvm;
using namespace model;
using namespace builder;
using namespace builder::mvll;

bool LLVMJitBuilder::Resolver::trace = false;
spug::Tracer LLVMJitBuilder::Resolver::tracer(
    "SymbolResolver",
    LLVMJitBuilder::Resolver::trace,
    "Resolution of unresolved symbols in cyclic modules in the LLVM jit."
);

void LLVMJitBuilder::Resolver::mergeCycleGroups(Module *a, Module *b) {
    if (trace)
        cerr << "merging cycle groups for " <<
            a->getModuleIdentifier() << " and " << b->getModuleIdentifier() <<
            endl;
    ModuleSet *&aSet = cycleMap[a];
    ModuleSet *&bSet = cycleMap[b];

    // if both sets are empty, create a new set containing both modules.
    if (!aSet && !bSet) {
        aSet = bSet = new ModuleSet();
        aSet->insert(a);
        aSet->insert(b);
        return;
    }

    // if the sets are non-empty but equal, they're already in the same set.
    if (aSet == bSet)
        return;

    // if either set is empty, just use the non-empty one.
    if (!aSet) {
        aSet = bSet;
        bSet->insert(a);
        return;
    }
    if (!bSet) {
        bSet = aSet;
        aSet->insert(b);
        return;
    }

    // We have two separate sets, merge them into one.
    for (ModuleSet::iterator i = bSet->begin();
         i != bSet->end();
         ++i
         )
        aSet->insert(*i);
    delete bSet;
    bSet = aSet;
}

void LLVMJitBuilder::Resolver::linkCyclicGroup(LLVMJitBuilder *builder,
                                               Module *module
                                               ) {
    if (trace)
        cerr << "Checking module group for " <<
            module->getModuleIdentifier() << endl;;
    ModuleSet *group = cycleMap[module];
    if (!group)
        return;
    for (ModuleSet::iterator i = group->begin(); i != group->end();
         ++i
         ) {
        if (!unresolvedMap[*i].empty()) {
            if (trace)
                cerr << "  Module " <<
                    (*i)->getModuleIdentifier() <<
                    " still has unresolved symbols.";
            return;
        }
    }

    // merge the modules.
    if (trace)
        cerr << "Linking cyclic module group: " << endl;
    ModuleMerger merger("cylic-module");
    for (ModuleSet::iterator i = group->begin(); i != group->end();
         ++i
         ) {
        if (trace)
            cerr << "  " << (*i)->getModuleIdentifier() << endl;;
//        string errMsg;
        merger.merge(*i);
//        if (errMsg.size())
//            cerr << "Error linking " << (*i)->getModuleIdentifier() <<
//                ": " << errMsg << endl;
        unresolvedMap.erase(*i);
        cycleMap.erase(*i);
    }

    // Map all unresolved externals in the composite module.

    // Global vars.
    builder->module = merger.getTarget();
    if (ModuleMerger::trace)
        builder->module->dump();
    for (Module::global_iterator iter = builder->module->global_begin();
        iter != builder->module->global_end();
        ++iter
        ) {
        if (iter->isDeclaration())
            SPUG_CHECK(resolve(builder->execEng, iter),
                       "global var " << iter->getName().str() <<
                        " remains unresolved."
                       );
        deferred.erase(iter->getName().str());
    }

    // Functions.
    for (Module::iterator iter = builder->module->begin();
        iter != builder->module->end();
        ++iter
        ) {
        // LLVM seams to occassionally create duplicate definitions of some
        // external definitions with the standard numeric suffix as a
        // disambiguator. Ignore these.  They're not used and can't be
        // resolved.
        string name = iter->getName().str();
        if (isdigit(name.substr(name.size() - 1)[0]))
            continue;

        if (iter->isDeclaration() && !iter->isMaterializable() &&
            !iter->isIntrinsic() &&
            (builder->getShlibSyms().find(iter->getName().str()) ==
             builder->getShlibSyms().end()
             )
            )
            SPUG_CHECK(resolve(builder->execEng, iter),
                       "function " << iter->getName().str() <<
                        " remains unresolved."
                       );
        deferred.erase(iter->getName().str());
    }

    // replace the current modules with the new module, do global
    // registration.
    builder->registerGlobals();

    // Fix the deferred types.
    for (ModuleSet::iterator i = group->begin(); i != group->end(); ++i) {
        TypeMap::iterator typesForModuleIter = deferredTypes.find(*i);
        if (typesForModuleIter == deferredTypes.end())
            continue;

        TypeVec &typesForModule = typesForModuleIter->second;
        for (TypeVec::iterator typeIter = typesForModule.begin();
             typeIter != typesForModule.end();
             ++typeIter
             ) {
            SPUG_CHECK((*typeIter)->getOwner(),
                       "Type " << (*typeIter)->name <<
                        " remains unowned at deferred type resolution."
                       );
            Type *type =
                builder->module->getTypeByName((*typeIter)->getFullName());
            SPUG_CHECK(type,
                       "Type " << (*typeIter)->getFullName() <<
                        "could not be resolved after cyclic module "
                        "integration."
                       );
            (*typeIter)->rep = type;
            if (trace)
                cerr << "Resolved deferred type " <<
                    (*typeIter)->getFullName() << endl;
        }

        // Remove the module from deferred tupes, we're done with it.
        deferredTypes.erase(typesForModuleIter);
    }

    // Back-patch the new module into the rep of all of the ModuleDef
    // objects that reference it.
    for (ModuleSet::iterator i = group->begin(); i != group->end();
         ++i
         ) {
        SourceMap::iterator j = sourceMap.find(*i);
        SPUG_CHECK(j != sourceMap.end(),
                   "No source map entry found for module " <<
                    (*i)->getModuleIdentifier()
                   );
        j->second->rep = builder->module;
        sourceMap.erase(j);
    }

    delete group;
}

bool LLVMJitBuilder::Resolver::resolveFixups(LLVMJitBuilder *builder,
                                             GlobalValue *globalVal,
                                             const string &name
                                             ) {
    Module *ownerMod = globalVal->getParent();

    // if there are fixups, go through them and coalesce the cycle groups
    FixupMap::iterator i = fixupMap.find(name);
    if (i != fixupMap.end()) {

        // This will remain true if this is the last unresolved symbol for all
        // of the modules that it is unresolved in, in which case we want to
        // check to see if it's time to link them at the end.
        bool checkForResolution = true;

        // XXX what this doesn't take into account is the modules that have
        // dependencies on symbols that are merely deferred.  Deferrred
        // symbols are part of the same cycle group.
        for (ModuleSet::iterator j = i->second.begin();
             j != i->second.end();
             ++j
             ) {
            mergeCycleGroups(ownerMod, *j);

            // remove the variable from the module's fixup map
            UnresolvedMap::iterator ur = unresolvedMap.find(*j);
            SPUG_CHECK(ur != unresolvedMap.end(),
                       "Module " << (*j)->getModuleIdentifier() <<
                           " is listed in fixups for " << name <<
                           " but isn't listed in the unresolved map."
                       );

            SPUG_CHECK(ur->second.erase(name) == 1,
                       "Module " << (*j)->getModuleIdentifier() <<
                            " is listed in fixups for " << name <<
                            " but doesn't have an entry for it in its "
                            "unresolved list."
                       );
            if (!ur->second.empty())
                checkForResolution = false;
        }
        fixupMap.erase(i);
        return checkForResolution;
    } else {
        return false;
    }
}

void LLVMJitBuilder::Resolver::registerGlobal(LLVMJitBuilder *builder,
                                              GlobalValue *globalVal
                                              ) {
    const string &name = globalVal->getName().str();
    cacheMap.insert(CacheMap::value_type(name, globalVal));
    // XXX bring back our check?
}

namespace {
    void checkReplacementType(GlobalValue *val, GlobalValue *replacement) {
        if (val->getType() != replacement->getType()) {
            cerr << "Replacing " << val->getName().str() <<
                " of type:" << endl;
            val->getType()->dump();
            cerr << "\nWith value of type:" << endl;
            replacement->getType()->dump();
        }
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

    // see if the symbol is merely deferred.
    DeferredMap::iterator di = deferred.find(name);
    if (di != deferred.end()) {
        // make sure that the modules are in the same cycle group.
        mergeCycleGroups(globalVal->getParent(), di->second);
        return false;
    }

    // the symbol hasn't been discovered yet, add its owner to the fixups.
    if (trace)
        cerr << "undiscovered " << globalVal->getName().str() << endl;
    Module *module = globalVal->getParent();
    fixupMap[name].insert(module);
    unresolvedMap[module].insert(name);
    return false;
}

bool LLVMJitBuilder::Resolver::defineGlobal(LLVMJitBuilder *builder,
                                            GlobalValue *globalVal,
                                            bool defer
                                            ) {
    bool triggerCheck = false;
    if (!globalVal->isDeclaration() || globalVal->isMaterializable()) {
        const string &name = globalVal->getName().str();
        triggerCheck = resolveFixups(builder, globalVal, name);
        if (defer) {
            if (trace) cerr << "deferring symbol " << name << endl;
            deferred.insert(make_pair(name, globalVal->getParent()));
        }
    }
    return triggerCheck;
}

void LLVMJitBuilder::Resolver::defineAll(LLVMJitBuilder *builder,
                                         BModuleDef *modDef,
                                         bool defer
                                         ) {
    Module *module = modDef->rep;

    // We _always_ insert into the source map, even if we're deferring,
    // because even if this module has no unresolved externals it's going to
    // get caught up in the cycle so we'll need to fix its rep after linking.
    sourceMap.insert(make_pair(module, modDef));

    // If we're deferring, add the module to the unresolved map so
    // isUnresolved() shows it as having unresolved externals.
    if (defer)
        unresolvedMap[module];

    bool triggerCheck = false;
    for (Module::iterator i = module->begin(); i != module->end(); ++i) {
        if (!i->isIntrinsic() && defineGlobal(builder, i, defer))
            triggerCheck = true;
    }

    for (Module::global_iterator i = module->global_begin();
         i != module->global_end();
         ++i
         ) {
        if (defineGlobal(builder, i, defer))
            triggerCheck = true;
    }

    // if a check was triggered, see if all of the modules in the group have
    // everything resolved.
    if (triggerCheck)
        linkCyclicGroup(builder, module);
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

bool LLVMJitBuilder::Resolver::isUnresolved(Module *module) {
    return unresolvedMap.find(module) != unresolvedMap.end();
}

void LLVMJitBuilder::Resolver::deferType(Module *module, BTypeDef *type) {
    if (trace)
        cerr << "Deferring type " << module->getModuleIdentifier() <<
            "." << type->name << endl;;
    deferredTypes[module].push_back(type);
}

bool LLVMJitBuilder::Resolver::hasActiveCycles() const {
    return fixupMap.size();
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
            resolver->registerGlobal(this, iter);
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
        passMan.add(new DataLayout(*execEng->getDataLayout()));
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
            resolver->registerGlobal(this, iter);
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
        resolver->registerGlobal(this, func->getFuncRep(*this));
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
    // In the case of a finished module, we must be able to jit it and get the
    // address.  We force jitting of all functions and globals in
    // registerGlobals(), which is called when the module is closed, so we
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
    execEng->updateGlobalMapping(pointer, real);
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
        resolver->registerGlobal(this,
                                 BFuncDefPtr::rcast(result)->getFuncRep(*this)
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

    doRunOrDump(context);

    // and if we're caching, store it in the persistent cache.
    if (moduleDef->cacheable && context.construct->cacheMode)
        context.cacheModule(moduleDef);

}

namespace {

    // Fix the reps for all functions in the overload to reference the correct
    // reps in the new module.
    void fixOverloadReps(OverloadDef *ovld, Module *module) {
        for (OverloadDef::FuncList::iterator i = ovld->beginTopFuncs();
             i != ovld->endTopFuncs();
             ++i
             ) {
            BFuncDef *func = BFuncDefPtr::rcast(*i);

            // ignore aliases, abstract methods and operations (ops don't
            // have a rep). There is also at least one operation that is not
            // a BFuncDef, so ignore that too.
            if (!func || func->getOwner() != ovld->getOwner() ||
                func->flags & FuncDef::abstract ||
                OpDefPtr::cast(func)
                )
                continue;

            Function *rep = module->getFunction((*i)->getUniqueId(0));
            SPUG_CHECK(rep,
                       "No rep found for function " << (*i)->getFullName()
                       );
            func->setRep(rep);
        }
    }

    // Recursively fix all global variable and function reps in the given
    // namespace to reference the correct reps in the new module.
    // We do this as a fixup after merging a batch of cyclics.
    void fixNamespaceReps(Namespace *ns, Module *module) {
        for (Namespace::VarDefMap::iterator def = ns->beginDefs();
             def != ns->endDefs();
             ++def
             ) {
            // ignore aliases.
            if (def->second->getOwner() != ns)
                continue;

            // Global variables and types.
            if (BGlobalVarDefImpl *imp =
                 BGlobalVarDefImplPtr::rcast(def->second->impl)
                ) {
                imp->setRep(
                    module->getGlobalVariable(def->second->getFullName())
                );

                // Types also have a global variable impl, but we need to also
                // treat them like namespaces.
                if (BTypeDef *type = BTypeDefPtr::rcast(def->second))
                    fixNamespaceReps(type, module);

            // Overloads.
            } else if (OverloadDef *ovld =
                        OverloadDefPtr::rcast(def->second)
                       ) {
                fixOverloadReps(ovld, module);

            // Misc. namespaces (some day we will have these).
            } else if (Namespace *nested = NamespacePtr::rcast(def->second)) {
                fixNamespaceReps(nested, module);
            }
        }

    }
}

void LLVMJitBuilder::mergeAndRegister(const vector<BJitModuleDefPtr> &modules) {
    if (modules.size() != 1) {
        ModuleMerger merger("cyclic-modules", execEng);
        for (vector<BJitModuleDefPtr>::const_iterator i = modules.begin();
             i != modules.end();
             ++i
             )
            merger.merge((*i)->rep);

        module = merger.getTarget();

        // replace the rep of the original source modules and all of their
        // contents with the those of the new, mega-module
        for (vector<BJitModuleDefPtr>::const_iterator i = modules.begin();
             i != modules.end();
             ++i
             ) {
            fixNamespaceReps(i->get(), module);
            (*i)->rep = module;
        }

        // ModuleMerge doesn't copy meta-data, so we have to add the
        // "finished" marker back to the new module.
        moduleDef->rep->getOrInsertNamedMetadata("crack_finished");
    }

    registerGlobals();
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

    resolver->registerGlobal(this, rep);
    SPUG_CHECK(varDef->getFullName() == rep->getName().str(),
               "global def " << varDef->getFullName() <<
                " doees not have the same name as its rep: " <<
                rep->getName().str()
               );

}

void LLVMJitBuilder::registerGlobals() {
    // register debug info for the module functions
    for (Module::iterator iter = module->begin();
         iter != module->end();
         ++iter
         ) {
        if ((!iter->isDeclaration() || iter->isMaterializable()) &&
            !iter->isIntrinsic()
            ) {
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
                resolver->registerGlobal(this, iter);
        }
    }

    // ... and global variables
    for (Module::global_iterator iter = module->global_begin();
         iter != module->global_end();
         ++iter
         ) {
        // force the global pointer into existence, this is just so we can
        // rely on the fact that this pointer exists once registerGlobals()
        // has been called.
        execEng->getPointerToGlobal(iter);
        if (!iter->isDeclaration()) {
            if (resolver)
                resolver->registerGlobal(this, iter);
        }
    }
}

TypeDefPtr LLVMJitBuilder::materializeType(Context &context,
                                           const string &name
                                           ) {
    if (resolver->isUnresolved(module)) {
        BTypeDefPtr type = new BTypeDef(0 /*metaType*/, name,
                                        0 /*llvmType*/,
                                        true, /*is pointer type*/
                                        0 /*nextVTableSlot*/
                                        );
        resolver->deferType(module, type.get());
        return type;
    } else {
        return LLVMBuilder::materializeType(context, name);
    }
}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(
    Context &context,
    const string &canonicalName,
    ModuleDef *owner
) {

    Cacher c(context, options.get());
    BJitModuleDefPtr bmod = c.maybeLoadFromCache(canonicalName);

    if (bmod) {

        // we materialized a module from bitcode cache
        // find the main function
        module = bmod->rep;

        // convert all of the known types in the module to their existing
        // instances
        {
            StructResolver structResolver(module);
            structResolver.buildTypeMap();
            structResolver.run();
        }

        // entry function
        func = c.getEntryFunction();

        engineBindModule(bmod.get());
        ensureResolver();

        // register "__CrackExceptionPersonality" and "_Unwind_Resume"
        if (context.construct->cacheMode) {

            LLVMContext &lctx = getGlobalContext();
            resolver->registerGlobal(this, getExceptionPersonalityFunc());
            resolver->registerGlobal(this, getUnwindResumeFunc());
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
            if (iter->isDeclaration() && !iter->isMaterializable() &&
                !iter->isIntrinsic() &&
                (getShlibSyms().find(iter->getName().str()) ==
                 getShlibSyms().end())
                )
                if (!resolver->resolve(execEng, iter))
                    hasUnresolvedExternals = true;
        }

        if (hasUnresolvedExternals) {
            resolver->defineAll(this, bmod.get(), true);
        } else {
            // If we there are modules in cycle resolution, define all of our
            // globals with the resolver (but don't mark them as deferred).
            if (resolver->hasActiveCycles())
                resolver->defineAll(this, bmod.get(), false);

            registerGlobals();
        }

        setupCleanup(bmod.get());

        doRunOrDump(context);

    }

    moduleDef = bmod;
    return bmod;
}
