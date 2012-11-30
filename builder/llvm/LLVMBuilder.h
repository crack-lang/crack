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
#include <llvm/Support/IRBuilder.h>
#include "builder/Builder.h"
#include "BTypeDef.h"
#include "BBuilderContextData.h"

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

SPUG_RCPTR(BHeapVarDefImpl);
class DebugInfo;
class FuncBuilder;
class BModuleDef;
SPUG_RCPTR(LLVMBuilder);

class LLVMBuilder : public Builder {
    private:
        typedef std::map<std::string, llvm::StructType *> TypeMap;
        static TypeMap llvmTypes;

    protected:

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
            if (rootBuilder)
                return rootBuilder->exStructType.get();
            else
                return exStructType.get();
        }

        /**
         * Creates a new LLVM module and initializes all of the exception 
         * handling declarations that need to be present.  Assigns the 
         * 'module' instance variable to the new module.
         */
        void createLLVMModule(const std::string &name);

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
         * common module initialization that happens in all builders
         * during initializeImport
         */
        void initializeImportCommon(model::ModuleDef* m,
                                    const model::ImportedDefVec &symbols
                                    );

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

    public:
        // currently experimenting with making these public to give objects in 
        // LLVMBuilder.cc's anonymous internal namespace access to them.  It 
        // seems to be cutting down on the amount of code necessary to do this.
        BModuleDef *bModDef;
        llvm::Module *module;
        llvm::Function *func;
        llvm::PointerType *llvmVoidPtrType;
        llvm::IRBuilder<> builder;
        llvm::Value *lastValue;
        llvm::BasicBlock *funcBlock;
        llvm::Function *exceptionPersonalityFunc;

        // keeps track of the Function object for the FuncDef in the builder's
        // module.
        typedef std::map<model::FuncDef *, llvm::Function *> ModFuncMap;
        ModFuncMap moduleFuncs;

        // keeps track of the GlobalVariable object for the VarDef in the
        // builder's module.
        typedef std::map<model::VarDefImpl *, llvm::GlobalVariable *> ModVarMap;
        ModVarMap moduleVars;

        static int argc;
        static char **argv;

        // the list of types that need to be fixed when the meta-class has 
        // been defined.
        std::vector<BTypeDefPtr> deferMetaClass;

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

        void setModFunc(model::FuncDef *funcDef, llvm::Function *func) {
            moduleFuncs[funcDef] = func;
        }

        llvm::Function *getModFunc(model::FuncDef *funcDef,
                                   llvm::Function *funcRep);

        void setModVar(model::VarDefImpl *varDef, llvm::GlobalVariable *var) {
            moduleVars[varDef] = var;
        }

        llvm::GlobalVariable *getModVar(model::VarDefImpl *varDef,
                                        llvm::GlobalVariable *gvar
                                        );
        
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
        virtual llvm::ExecutionEngine *getExecEng() const = 0;

        LLVMBuilder();

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

        virtual model::VarDefPtr materializeVar(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type
        );

        virtual model::ArgDefPtr materializeArg(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type
        );

        virtual model::TypeDefPtr materializeType(
            model::Context &context,
            const std::string &name
        );

        virtual model::FuncDefPtr materializeFunc(
            model::Context &context,
            const std::string &name,
            const model::ArgVec &args
        );

        virtual void cacheModule(
            model::Context &context,
            model::ModuleDef *module
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

        virtual void *loadSharedLibrary(const std::string &name);

        virtual void importSharedLibrary(const std::string &name,
                                         const model::ImportedDefVec &symbols,
                                         model::Context &context,
                                         model::Namespace *ns
                                         );
        virtual void registerImportedDef(model::Context &context,
                                         model::VarDef *varDef
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
