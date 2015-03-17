// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_LLVMBuilder_h_
#define _builder_LLVMBuilder_h_

#include <map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/Support/MemoryBuffer.h>

#include "builder/Builder.h"
#include "BTypeDef.h"
#include "BBuilderContextData.h"
#include "BModuleDef.h"
#include "Cacher.h"

namespace llvm {
    class Module;
    class Function;
    class BasicBlock;
    class Type;
    class Value;
    class Function;
};

namespace builder {
namespace mvll {

class BFuncDef;
class BGlobalVarDefImpl;
SPUG_RCPTR(BHeapVarDefImpl);
class DebugInfo;
class FuncBuilder;
SPUG_RCPTR(LLVMBuilder);

class LLVMBuilder : public Builder {
    private:
        struct SharedLibDef : public model::ModuleDef {
            void *handle;
            SharedLibDef(const std::string &name, void *handle) : 
                ModuleDef(name, 0),
                handle(handle) {
            }
            virtual void callDestructor() {}
            virtual void runMain(Builder &builder) {}
        };
        SPUG_RCPTR(SharedLibDef);
        typedef std::map<std::string, llvm::StructType *> TypeMap;
        static TypeMap llvmTypes;
        llvm::Function *exceptionPersonalityFunc, *unwindResumeFunc;
        typedef std::map<std::string, SharedLibDefPtr> SharedLibMap;
        SharedLibMap sharedLibs;
        SharedLibMap &getSharedLibs() {
            return getRoot().sharedLibs;
        }
        
    protected:

        class LLVMCacheFile : public CacheFile {
            private:
                Cacher cacher;
                std::string canonicalName;

            public:
                LLVMCacheFile(model::Context &context, 
                              BuilderOptions *options,
                              const std::string &canonicalName
                              ) :
                    cacher(context, options),
                    canonicalName(canonicalName) {
                }
                llvm::OwningPtr<llvm::MemoryBuffer> fileBuf;
                
                bool getCacheFile() {
                    return cacher.getCacheFile(canonicalName, fileBuf);
                }
                
                BModuleDefPtr maybeLoadFromCache() {
                    return cacher.maybeLoadFromCache(canonicalName, fileBuf);
                }
        };
        SPUG_RCPTR(LLVMCacheFile);

        llvm::Function *callocFunc;
        DebugInfo *debugInfo;
        BTypeDefPtr exStructType;
        
        // emit all cleanups for context and all parent contextts up to the 
        // level of the function
        void emitFunctionCleanups(model::Context &context);
        
        // stores primitive function pointers
        std::map<llvm::Function *, void *> primFuncs;
        
        LLVMBuilderPtr rootBuilder;
        
        BTypeDef *getExStructType() {
            return getRoot().exStructType.get();
        }

        /**
         * Creates a new LLVM module and initializes all of the exception 
         * handling declarations that need to be present.  Assigns the 
         * 'module' instance variable to the new module.
         */
        void createLLVMModule(const std::string &name);

        /**
         * Finish generating the top-level module functions (:main and 
         * :cleanup)
         */
        void finishModule(model::Context &context, model::ModuleDef *modDef);

        void initializeMethodInfo(model::Context &context, 
                                  model::FuncDef::Flags flags,
                                  model::FuncDef *existing,
                                  BTypeDef *&classType,
                                  FuncBuilder &funcBuilder
                                  );

        /**
         * JIT builder uses this to map GlobalValue* to their JITed versions,
         * Linked builder ignores this, as these are resolved during the link
         * instead. It is called from getModFunc and getModVar
         */
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          llvm::Function*) { }
        virtual void addGlobalFuncMapping(llvm::Function*,
                                          void*) { }

        /**
         * Lets the JIT builder keep track of shared library symbols.
         */
        virtual void recordShlibSym(const std::string &name) {}

        /**
         * possibly bind the module to an execution engine
         * called in base llvmbuilder only from registerPrimFuncs
         */
        virtual void engineBindModule(BModuleDef *moduleDef) { }

        /**
         * let the engine "finish" a module before running/linking/dumping
         * called in base llvmbuilder only from registerPrimFuncs
         */
        virtual void engineFinishModule(model::Context &context,
                                        BModuleDef *moduleDef
                                        ) {
        }
        
        /**
         * common module initialization that happens in all builders
         * during createModule. includes some functions that need to be
         * defined in each module.
         */
        void createModuleCommon(model::Context &context);

        /**
          * get a realpath source path for the module
          */
        std::string getSourcePath(const std::string &path);

        /**
         * Gets the first unwind block for the context, emitting the whole 
         * cleanup chain if necessary.
         */
        llvm::BasicBlock *getUnwindBlock(model::Context &context);

        /**
         * Returns the exception personality function for the current builder 
         * (and hence, the current module).
         */        
        llvm::Function *getExceptionPersonalityFunc();
        
        /**
         * Clears all cached cleanup blocks associated with the context (this 
         * exists to deal with the module level init and delete functions, 
         * which both run against the module context).
         */
        void clearCachedCleanups(model::Context &context);

        /** Creates special hidden variables used by the generated code. */
        void createSpecialVar(model::Namespace *ns,
                              model::TypeDef *type,
                              const std::string &name
                              );

        /** Creates the "start blocks" for the current function. */
        void createFuncStartBlocks(const std::string &name);
        
        /** Emit the beginning of the module :main function. */
        void beginModuleMain(const std::string &name);

        /**
         * Delegates actual creation of the module object to derived classes.
         */
        virtual model::ModuleDefPtr innerCreateModule(model::Context &context,
                                                      const std::string &name,
                                                      model::ModuleDef *owner
                                                      ) = 0;

        /** 
         * Create a following block and cleanup block for an Invoke 
         * instruction given the context.
         */
        void getInvokeBlocks(model::Context &context, 
                             llvm::BasicBlock *&followingBlock,
                             llvm::BasicBlock *&cleanupBlock
                             );
        /** 
         * Insures that the class body global is present in the current module.
         */
        virtual void fixClassInstRep(BTypeDef *type) = 0;

        // The module id to use for the next module.  This is a monotonically 
        // increasing value which is used to verify that the rep of a def is 
        // associated with the current module.
        // This is also maintained by the root module.
        int nextModuleId;
        int getNextModuleId();
                
    public:
        // currently experimenting with making these public to give objects in 
        // LLVMBuilder.cc's anonymous internal namespace access to them.  It 
        // seems to be cutting down on the amount of code necessary to do this.
        BModuleDefPtr moduleDef;
        llvm::Module *module;
        llvm::Function *func;
        llvm::PointerType *llvmVoidPtrType;
        llvm::IRBuilder<> builder;
        llvm::Value *lastValue;
        llvm::Type *intzLLVM;
        llvm::BasicBlock *funcBlock;
        
        /** 
         * Returns the root builder or the current builder if it is the root. 
         */
        LLVMBuilder &getRoot() {
            return rootBuilder ? *rootBuilder : *this;
        }

        const LLVMBuilder &getRoot() const {
            return rootBuilder ? *rootBuilder : *this;
        }
        
        static int argc;
        static char **argv;

        // the list of types that need to be fixed when the meta-class has 
        // been defined.
        std::vector<BTypeDefPtr> deferMetaClass;

        /** Gets the _Unwind_Resume function. */
        llvm::Function *getUnwindResumeFunc();
        
        /**
         * Instantiates the BModuleDef subclass appropriate for the builder.
         */
        virtual BModuleDef *instantiateModule(model::Context &context,
                                              const std::string &name,
                                              llvm::Module *module
                                              );

        /** 
         * Returns true if cleanups should be suppressed (i.e. after a throw) 
         */
        bool suppressCleanups();

        void narrow(model::TypeDef *curType, model::TypeDef *ancestor);

        llvm::Function *getModFunc(BFuncDef *funcDef);

        llvm::GlobalVariable *getModVar(BGlobalVarDefImpl *varDef);
        
        BTypeDefPtr getFuncType(model::Context &context,
                                model::FuncDef *funcDef,
                                llvm::Type *llvmFuncType
                                );
        BHeapVarDefImplPtr createLocalVar(BTypeDef *tp,
                                          llvm::Value *&var,
                                          const std::string &name,
                                          const parser::Location *loc = 0,
                                          llvm::Value *initVal = 0
                                          );
        
        BTypeDefPtr createClass(model::Context &context,
                                const std::string &name,
                                unsigned int nextVTableSlot
                                );

        virtual void *getFuncAddr(llvm::Function *func) = 0;

        /**
         * Gives LLVMJitBuilder a chance to keep track of orphaned defs for 
         * module merge.
         */
        virtual void recordOrphanedDef(model::VarDef *def) {}

        /** Creates an expresion to cleanup the current exception. */
        void emitExceptionCleanupExpr(model::Context &context);

        /** 
         * Create a landing pad block. 
         * @param next the block after the landing pad
         * @param cdata catch data or null if this is not a catch context.
         */
        llvm::BasicBlock *createLandingPad(
            model::Context &context,
            llvm::BasicBlock *next,
            BBuilderContextData::CatchData *cdata
        );

        virtual void addGlobalVarMapping(llvm::GlobalValue *decl,
                                         llvm::GlobalValue *externalDef
                                         ) {
        }

        /** Return the execution engine if there is one, null if not. */        
        virtual llvm::ExecutionEngine *getExecEng() = 0;

        LLVMBuilder(LLVMBuilder *root = 0);

        virtual model::ResultExprPtr emitFuncCall(
            model::Context &context, 
            model::FuncCall *funcCall
        );
        
        virtual model::ResultExprPtr emitStrConst(model::Context &context,
                                                  model::StrConst *strConst
                                                  );

        virtual model::ResultExprPtr emitIntConst(model::Context &context,
                                                  model::IntConst *val
                                                  );
        virtual model::ResultExprPtr emitFloatConst(model::Context &context,
                                                  model::FloatConst *val
                                                  );

        virtual model::ResultExprPtr emitNull(model::Context &context,
                                              model::NullConst *nullExpr
                                              );

        virtual model::ResultExprPtr emitAlloc(model::Context &context, 
                                               model::AllocExpr *allocExpr,
                                               model::Expr *countExpr
                                               );

        virtual void emitTest(model::Context &context,
                              model::Expr *expr
                              );

        virtual model::BranchpointPtr emitIf(model::Context &context,
                                             model::Expr *cond);

        model::BranchpointPtr labeledIf(model::Context &context,
                                        model::Expr *cond,
                                        const char* tLabel=0,
                                        const char* fLabel=0,
                                        bool condInCleanupFrame=true
                                        );
        
        virtual model::BranchpointPtr
            emitElse(model::Context &context,
                     model::Branchpoint *pos,
                     bool terminal
                     );
        
        virtual void emitEndIf(model::Context &context,
                               model::Branchpoint *pos,
                               bool terminal
                               );

        virtual model::TernaryExprPtr createTernary(model::Context &context,
                                                    model::Expr *cond,
                                                    model::Expr *trueVal,
                                                    model::Expr *falseVal,
                                                    model::TypeDef *type
                                                    );

        virtual model::ResultExprPtr emitTernary(model::Context &context,
                                                 model::TernaryExpr *expr
                                                 );

        virtual model::BranchpointPtr 
            emitBeginWhile(model::Context &context, 
                           model::Expr *cond,
                           bool gotPostBlock
                           );

        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  );

        virtual void emitPostLoop(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  );

        virtual void emitBreak(model::Context &context, 
                               model::Branchpoint *branch
                               );

        virtual void emitContinue(model::Context &context, 
                                  model::Branchpoint *branch
                                  );

        virtual model::BranchpointPtr emitBeginTry(model::Context &context);
        
        virtual model::ExprPtr emitCatch(model::Context &context,
                                         model::Branchpoint *branchpoint,
                                         model::TypeDef *catchType,
                                         bool terminal
                                         );
        
        virtual void emitEndTry(model::Context &context,
                                model::Branchpoint *branchpoint,
                                bool terminal
                                );

        virtual void emitExceptionCleanup(model::Context &context);

        virtual void emitThrow(model::Context &context,
                               model::Expr *expr
                               );

        virtual model::FuncDefPtr
            createFuncForward(model::Context &context,
                              model::FuncDef::Flags flags,
                              const std::string &name,
                              model::TypeDef *returnType,
                              const std::vector<model::ArgDefPtr> &args,
                              model::FuncDef *override
                              );

        virtual model::TypeDefPtr
            createClassForward(model::Context &context,
                               const std::string &name
                               );

        virtual model::FuncDefPtr
            emitBeginFunc(model::Context &context,
                          model::FuncDef::Flags flags,
                          const std::string &name,
                          model::TypeDef *returnType,
                          const std::vector<model::ArgDefPtr> &args,
                          model::FuncDef *existing
                          );
        
        virtual void emitEndFunc(model::Context &context,
                                 model::FuncDef *funcDef);

        model::FuncDefPtr createExternFuncCommon(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            model::TypeDef *receiverType,
            const std::vector<model::ArgDefPtr> &args,
            void *cfunc,
            const char *symbolName
        );

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

        virtual model::TypeDefPtr
            emitBeginClass(model::Context &context,
                           const std::string &name,
                           const std::vector<model::TypeDefPtr> &bases,
                           model::TypeDef *forwardDef
                           );

        virtual void emitEndClass(model::Context &context);

        virtual void emitReturn(model::Context &context,
                                model::Expr *expr);

        virtual model::VarDefPtr emitVarDef(model::Context &container,
                                            model::TypeDef *type,
                                            const std::string &name,
                                            model::Expr *initializer,
                                            bool staticScope
                                            );

        virtual model::VarDefPtr createOffsetField(model::Context &context,
                                                   model::TypeDef *type,
                                                   const std::string &name,
                                                   size_t offset
                                                   );

        // for definitions, we're going to just let the builder be a factory 
        // so that it can tie whatever special information it wants to the 
        // definition.
        
        virtual model::FuncCallPtr
            createFuncCall(model::FuncDef *func, bool squashVirtual);
        virtual model::ArgDefPtr createArgDef(model::TypeDef *type,
                                              const std::string &name
                                              );
        virtual model::VarRefPtr createVarRef(model::VarDef *varDef);
        virtual model::VarRefPtr
            createFieldRef(model::Expr *aggregate,
                           model::VarDef *varDef
                           );
        virtual model::ResultExprPtr emitFieldAssign(model::Context &context,
                                                     model::Expr *aggregate,
                                                     model::AssignExpr *assign
                                                     );

        model::ModuleDefPtr createModule(model::Context &context, 
                                         const std::string &name,
                                         const std::string &path,
                                         model::ModuleDef *owner
                                         );

        virtual CacheFilePtr getCacheFile(model::Context &context,
                                          const std::string &canonicalName
                                          );

        virtual model::VarDefPtr materializeVar(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type,
            int instSlot
        );

        virtual model::ArgDefPtr materializeArg(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type
        );

        virtual model::TypeDefPtr materializeType(
            model::Context &context,
            const std::string &name,
            const std::string &namespaceName
        );

        virtual model::FuncDefPtr materializeFunc(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            const model::ArgVec &args
        );

        virtual void cacheModule(
            model::Context &context,
            model::ModuleDef *module,
            const std::string &uniquifier
        );

        virtual void finishCachedModule(
            model::Context &context,
            model::ModuleDef *module,
            const std::string &uniquifier,
            bool retain
        );
        
        virtual model::CleanupFramePtr
            createCleanupFrame(model::Context &context);
        virtual void closeAllCleanups(model::Context &context);
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val);
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  int64_t val,
                                                  model::TypeDef *type = 0
                                                  );
        virtual model::IntConstPtr createUIntConst(model::Context &context,
                                                   uint64_t val,
                                                   model::TypeDef *type = 0
                                                   );
        virtual model::FloatConstPtr createFloatConst(model::Context &context,
                                                  double val,
                                                  model::TypeDef *type = 0
                                                  );

        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context);
        
        virtual void initialize(model::Context &context);

        virtual void *loadSharedLibrary(const std::string &name);

        virtual void importSharedLibrary(const std::string &name,
                                         const model::ImportedDefVec &symbols,
                                         model::Context &context,
                                         model::Namespace *ns
                                         );
        virtual void registerImportedDef(model::Context &context,
                                         model::VarDef *varDef
                                         );
        void initializeImport(model::ModuleDef* m,
                              const model::ImportedDefVec &symbols
                              );


        // used by Cacher for maintaining a global (cross module)
        // cache map of vardefs. this is not part of the base Builder
        // interface
        virtual void registerDef(model::Context &context,
                                 model::VarDef *varDef
                                 ) { }

        virtual void setArgv(int argc, char **argv);        

        // internal functions used by our VarDefImpl to generate the 
        // appropriate variable references.
        void emitMemVarRef(model::Context &context, llvm::Value *val);
        void emitArgVarRef(model::Context &context, llvm::Value *val);
        
        // XXX hack to emit all vtable initializers until we get constructor 
        // composition.
        virtual void emitVTableInit(model::Context &context,
                                    model::TypeDef *typeDef
                                    );
        
        // functions to manage the global type table.  The global type table 
        // allows us to normalize types when loading modules from the 
        // persistent cache.
        
        /**
         * Return the LLVM type associated with the given canonical name.  
         * Returns null if there is currently no type stored under that name.
         */
        static llvm::StructType *getLLVMType(const std::string &canonicalName);
        
        /**
         * Store a type by name in the global LLVM type table.
         */
        static void putLLVMType(const std::string &canonicalName, 
                                llvm::StructType *type
                                );


};

} // namespace builder::mvll
} // namespace builder
#endif
