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
#include "model/Visitor.h"
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

namespace {

    class ModuleChangeVisitor : public Visitor {
        private:
            Module *oldMod, *newMod;

        public:
            ModuleChangeVisitor(Module *oldMod, Module *newMod) :
                oldMod(oldMod),
                newMod(newMod) {
            }

            virtual void onModuleDef(ModuleDef *module) {
                BJitModuleDefPtr::cast(module)->rep = newMod;
            }

            virtual void onTypeDef(TypeDef *type) {
                // We don't have to touch the 'rep' because types are global
                // and it should already be correct.
                BTypeDef *btype = BTypeDefPtr::cast(type);
                if (btype) {

                    // Ignore types that aren't in the old module.
                    if (btype->classInst->getParent() != oldMod)
                        return;

                    btype->classInst =
                        newMod->getGlobalVariable(btype->classInst->getName());

                    // We can discard the vtables at this point, they are no
                    // longer needed.
                    btype->vtables.clear();
                }

                // Types also have all of the global variable implementation
                // machinery, so we need to do onVarDef() on them, too.
                onVarDef(type);
            }

            virtual void onVarDef(VarDef *var) {
                // We just reset the rep for two of the types that are
                // sensitive to it: these reps get checked against the current
                // module, but there's a small chance that the module object
                // address could be reused.
                VarDefImpl *impl = var->impl.get();
                if (BGlobalVarDefImpl *glblImpl =
                     BGlobalVarDefImplPtr::cast(impl)
                    ) {
                    glblImpl->fixModule(oldMod, newMod);
                } else if (BConstDefImpl *cnstImpl =
                          BConstDefImplPtr::cast(impl)
                         ) {
                    cnstImpl->fixModule(oldMod, newMod);
                }
            }

            virtual void onOverloadDef(OverloadDef *ovld) {
                onVarDef(ovld);
            }

            virtual void onFuncDef(FuncDef *func) {
                BFuncDefPtr bfunc = BFuncDefPtr::cast(func);
                if (bfunc)
                    bfunc->fixModule(oldMod, newMod);
                onVarDef(func);
            }

    };
}

void LLVMJitBuilder::mergeModule(ModuleDef *moduleDef) {
    ModuleMerger *merger = getModuleMerger();
    merger->merge(module);

    // Now we need to fix the global variables, functions and type meta-data
    // references in everything...
    ModuleChangeVisitor visitor(module, merger->getTarget());
    moduleDef->visit(&visitor);

    // Do the orphaned var defs.
    SPUG_FOR(vector<VarDefPtr>, i, orphanedDefs)
        (*i)->visit(&visitor);

    // Add the module to the list of modules where we need to import the
    // cleanup.
    if (rootBuilder)
        LLVMJitBuilderPtr::rcast(rootBuilder)->needsCleanup.push_back(
            BModuleDefPtr::cast(moduleDef)
        );
    else
        needsCleanup.push_back(BModuleDefPtr::cast(moduleDef));
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

ModuleMerger *LLVMJitBuilder::getModuleMerger() {
    if (rootBuilder) {
        return LLVMJitBuilderPtr::rcast(rootBuilder)->getModuleMerger();
    } else if (!moduleMerger) {
        moduleMerger = new ModuleMerger("merged-modules", getExecEng());
        bindJitModule(moduleMerger->getTarget());
    }
    return moduleMerger;
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

void *LLVMJitBuilder::getFuncAddr(llvm::Function *func) {
    void *addr = execEng->getPointerToFunction(func);
    SPUG_CHECK(addr,
               "Unable to resolve function " << string(func->getName()));
    return addr;
}

void LLVMJitBuilder::recordOrphanedDef(VarDef *def) {
    orphanedDefs.push_back(def);
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
    return new BJitModuleDef(name, context.ns.get(), module,
                             BJitModuleDefPtr::cast(owner)
                             );

}

void LLVMJitBuilder::innerCloseModule(Context &context, ModuleDef *moduleDef) {
    finishModule(context, moduleDef);
// XXX in the future, only verify if we're debugging
//    if (debugInfo)
        verifyModule(*module, llvm::PrintMessageAction);

    // Do the common stuff (common with the .builtin module, which doesn't get
    // closed)
    innerFinishModule(context, BModuleDefPtr::cast(moduleDef));

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
    mergeModule(moduleDef);
    delete module;
    this->moduleDef->clearRepFromConstants();
    module = 0;
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
                // to fix their class instances and then treat them like
                // namespaces.
                if (BTypeDef *type = BTypeDefPtr::rcast(def->second)) {
                    type->classInst = module->getGlobalVariable(
                        type->classInst->getName()
                    );
                    fixNamespaceReps(type, module);
                }

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

void LLVMJitBuilder::registerGlobals() {
    // register debug info for the module functions
    Module *merged = getModuleMerger()->getTarget();
    for (Module::iterator iter = merged->begin();
         iter != merged->end();
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
        }
    }

    // While we're at it, do setupCleanup() on all of the modules on our list.
    LLVMJitBuilder *b = rootBuilder ? LLVMJitBuilderPtr::rcast(rootBuilder) :
                                      this;
    SPUG_FOR(std::vector<BModuleDefPtr>, i, b->needsCleanup)
        setupCleanup(i->get());
    b->needsCleanup.clear();
}

model::ModuleDefPtr LLVMJitBuilder::materializeModule(
    Context &context,
    const string &canonicalName,
    ModuleDef *owner
) {

    Cacher c(context, options.get());
    BJitModuleDefPtr bmod = c.maybeLoadFromCache(canonicalName);

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

        // entry function
        func = c.getEntryFunction();

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
        module = getModuleMerger()->getTarget();
    }

    return bmod;
}
