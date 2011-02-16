// Copyright 2009-2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_LLVMLinkerBuilder_h_
#define _builder_LLVMLinkerBuilder_h_

#include "LLVMBuilder.h"
#include <vector>

namespace llvm {
    class Linker;
    class BasicBlock;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(LLVMLinkerBuilder);

class LLVMLinkerBuilder : public LLVMBuilder {
    private:
        typedef std::vector<model::ModuleDef *> ModuleListType;

        llvm::Linker *linker;
        ModuleListType *moduleList;
        llvm::BasicBlock *mainInsert;
        std::vector<std::string> sharedLibs;

        llvm::Linker *linkModule(llvm::Module *mp);
        ModuleListType *addModule(model::ModuleDef *mp);

        llvm::Function *emitAggregateCleanup(llvm::Module *module);

    protected:
        virtual void engineFinishModule(model::ModuleDef *moduleDef);

    public:
        LLVMLinkerBuilder(void) : linker(0),
                                  moduleList(0),
                                  mainInsert(0),
                                  sharedLibs() { }

        virtual void *getFuncAddr(llvm::Function *func);

        virtual void finish(model::Context &context);

        virtual BuilderPtr createChildBuilder();

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name
                                                 );

        virtual void initializeImport(model::ModuleDefPtr, bool annotation);

        virtual void *loadSharedLibrary(const std::string &name);

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *module
                                 );

        virtual bool isExec() { return false; }

};

} } // namespace

#endif
