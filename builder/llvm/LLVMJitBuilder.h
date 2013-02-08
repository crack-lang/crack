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
        llvm::ExecutionEngine *execEng;

        llvm::ExecutionEngine *bindJitModule(llvm::Module *mp);
        
        std::vector< std::pair<llvm::Function *, llvm::Function *> > externals;

        typedef std::map<std::string, llvm::GlobalValue *> CacheMapType;
        CacheMapType *cacheMap;

        virtual void run();

        virtual void dump();

        void doRunOrDump(model::Context &context);

        void ensureCacheMap();

        void setupCleanup(BModuleDef *moduleDef);

        void buildDebugTables();
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

        LLVMJitBuilder(void) : execEng(0), cacheMap(0) { }

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
            if (cacheMap)
                delete cacheMap;
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
