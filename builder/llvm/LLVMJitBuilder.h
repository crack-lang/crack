// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_LLVMJitBuilder_h_
#define _builder_LLVMJitBuilder_h_

#include "LLVMBuilder.h"

#include "spug/Tracer.h"
#include "ModuleMerger.h"

namespace llvm {
    class ExecutionEngine;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(LLVMJitBuilder);

class LLVMJitBuilder : public LLVMBuilder {
    private:
        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);

        // symbols that were imported from a shared library (we don't want to 
        // try to resolve these).
        std::set<std::string> shlibSyms;
        std::set<std::string> &getShlibSyms() {
            if (rootBuilder)
                return LLVMJitBuilderPtr::rcast(rootBuilder)->shlibSyms;
            else
                return shlibSyms;
        }
        
        // List of modules that need to have their cleanups setup before we 
        // run main.
        std::vector<BModuleDefPtr> needsCleanup;

        ModuleMerger *moduleMerger;
        ModuleMerger *getModuleMerger();
        
        virtual void run();

        virtual void dump();

        void doRunOrDump(model::Context &context);

        void setupCleanup(BModuleDef *moduleDef);

        void fixupAfterMerge(model::ModuleDef *moduleDef, llvm::Module *merged);

    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);
        virtual void recordShlibSym(const std::string &name);

        virtual void engineBindModule(BModuleDef *moduleDef);

        // Contains most of the meat of engineFinishModule.
        void innerFinishModule(model::Context &context,
                               BModuleDef *moduleDef
                               );
        
        // Merge the module into the global main module, replace all of the 
        // old pointers into the original module (including the builder's) and 
        // delete it.
        void mergeModule(model::ModuleDef *moduleDef);

        virtual void engineFinishModule(model::Context &context,
                                        BModuleDef *moduleDef
                                        );
        virtual model::ModuleDefPtr innerCreateModule(model::Context &context,
                                                      const std::string &name,
                                                      model::ModuleDef *owner
                                                      );
        virtual void fixClassInstRep(BTypeDef *type);
        virtual BModuleDef *instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *owner
                                              );

    public:
        virtual void addGlobalVarMapping(llvm::GlobalValue *decl,
                                         llvm::GlobalValue *externalDef
                                         );

        LLVMJitBuilder(LLVMBuilder *root = 0) :
            LLVMBuilder(root),
            execEng(0), 
            moduleMerger(0) {
        }
        ~LLVMJitBuilder();

        virtual void *getFuncAddr(llvm::Function *func);
        virtual void recordOrphanedDef(model::VarDef *def);

        virtual model::FuncDefPtr
            createExternFunc(model::Context &context,
                             model::FuncDef::Flags flags,
                             const std::string &name,
                             model::TypeDef *returnType,
                             model::TypeDef *receiverType,
                             const std::vector<model::ArgDefPtr> &args,
                             void *cfunc,
                             const char *symbolName=0
                             );

        virtual BuilderPtr createChildBuilder();

        void innerCloseModule(model::Context &context, 
                              model::ModuleDef *module
                              );
        
        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

        virtual bool isExec() { return true; }

        virtual void finishBuild(model::Context &context) {}

        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 );

        /** Registers all of the cleanup functions. */
        void registerCleanups();

        virtual model::ModuleDefPtr materializeModule(
            model::Context &context,
            CacheFile *cacheFile,
            const std::string &canonicalName,
            model::ModuleDef *owner
        );
        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context);
        virtual void initialize(model::Context &context);
        virtual llvm::ExecutionEngine *getExecEng();
};

} } // namespace

#endif
