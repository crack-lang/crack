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

namespace llvm {
    class ExecutionEngine;
}

namespace builder {
namespace mvll {

class BModuleDef;
SPUG_RCPTR(LLVMJitBuilder);

class LLVMJitBuilder : public LLVMBuilder {
    private:
        
        class Resolver {
            private:
                // the cache map tracks symbols that we've already resolved.
                typedef std::map<std::string, llvm::GlobalValue *> CacheMap;
                CacheMap cacheMap;
        
                // the fixup map stores the modules that need a given symbol.
                typedef std::vector<llvm::GlobalValue *> GlobalValueVec;
                typedef std::map<std::string, GlobalValueVec> FixupMap;
                FixupMap fixupMap;
            
            public:
                void registerGlobal(llvm::ExecutionEngine *execEng,
                                    llvm::GlobalValue *globalVal
                                    );
                
                // Resolves the symbol if it's in the cache, otherwise adds a 
                // fix-up for it.  Returns true if it was able to resolve the 
                // symbol, false if not.
                bool resolve(llvm::ExecutionEngine *execEng,
                             llvm::GlobalValue *globalVal
                             );
        };

                
        
        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);
        
        std::vector< std::pair<llvm::Function *, llvm::Function *> > externals;
        
        Resolver *resolver;

        virtual void run();

        virtual void dump();

        void doRunOrDump(model::Context &context);

        void ensureResolver();

        void setupCleanup(BModuleDef *moduleDef);

        /**
         * Builds debug tables and registers all global symbols with the cache 
         * map.
         */
        void registerGlobals();
    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);

        virtual void engineBindModule(BModuleDef *moduleDef);
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

        LLVMJitBuilder(void) : execEng(0), resolver(0) { }

        virtual void *getFuncAddr(llvm::Function *func);


        virtual BuilderPtr createChildBuilder();

        void innerCloseModule(model::Context &context, 
                              model::ModuleDef *module
                              );

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

        virtual bool isExec() { return true; }

        virtual void finishBuild(model::Context &context) {
            if (resolver)
                delete resolver;
        }

        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 );

        virtual model::ModuleDefPtr materializeModule(
            model::Context &context,
            const std::string &canonicalName,
            model::ModuleDef *owner
        );
        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context);
        virtual llvm::ExecutionEngine *getExecEng();
};

} } // namespace

#endif
