// Copyright 2009 Google Inc., Shannon Weyrick <weyrick@mozek.us>

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

        void cacheModule(model::Context &context, model::ModuleDef *moduleDef);
    protected:
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*);
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*);

        virtual void engineBindModule(BModuleDef *moduleDef);
        virtual void engineFinishModule(BModuleDef *moduleDef);
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

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name,
                                                 const std::string &path,
                                                 model::ModuleDef *module
                                                 );

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

        virtual void initializeImport(model::ModuleDef* m,
                                      const std::vector<std::string> &symbols,
                                      bool annotation) {
            initializeImportCommon(m, symbols);
        }        
        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 );

        virtual model::ModuleDefPtr materializeModule(model::Context &context,
                                              const std::string &canonicalName,
                                              const std::string &path,
                                              model::ModuleDef *owner
                                              );
};

} } // namespace

#endif
