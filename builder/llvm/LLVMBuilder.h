// Copyright 2009 Google Inc.

#ifndef _builder_LLVMBuilder_h_
#define _builder_LLVMBuilder_h_

#include <map>

#include "builder/Builder.h"

#include <llvm/Support/IRBuilder.h>

namespace llvm {
    class Module;
    class Function;
    class BasicBlock;
    class Type;
    class Value;
    class Function;
    class ExecutionEngine;
};

namespace builder {
namespace mvll {

SPUG_RCPTR(BHeapVarDefImpl);
SPUG_RCPTR(BTypeDef);
class DebugInfo;
class FuncBuilder;
SPUG_RCPTR(LLVMBuilder);

class LLVMBuilder : public Builder {
    private:

        llvm::Function *callocFunc;
        DebugInfo *debugInfo;
        llvm::ExecutionEngine *execEng;
        
        // emit all cleanups for context and all parent contextts up to the 
        // level of the function
        void emitFunctionCleanups(model::Context &context);
        
        // stores primitive function pointers
        std::map<llvm::Function *, void *> primFuncs;
        
        // keeps track of the Function object for the FuncDef in the builder's 
        // module.
        typedef std::map<model::FuncDef *, llvm::Function *> ModFuncMap;
        ModFuncMap moduleFuncs;
        
        // mapping from pointer types to crack types (only exists in the root 
        // builder)
        typedef std::map<const llvm::Type *, model::TypeDefPtr> FuncTypeMap;
        FuncTypeMap funcTypes; 
        
        // keeps track of the GlobalVariable object for the VarDef in the 
        // builder's module.
        typedef std::map<model::VarDefImpl *, llvm::GlobalVariable *> ModVarMap;
        ModVarMap moduleVars;

        LLVMBuilderPtr rootBuilder;
        
        llvm::ExecutionEngine *bindModule(llvm::Module *mp);

        void initializeMethodInfo(model::Context &context, 
                                  model::FuncDef::Flags flags,
                                  model::FuncDef *existing,
                                  BTypeDef *&classType,
                                  FuncBuilder &funcBuilder
                                  );

    public:
        // currently experimenting with making these public to give objects in 
        // LLVMBuilder.cc's anonymous internal namespace access to them.  It 
        // seems to be cutting down on the amount of code necessary to do this.
        llvm::Module *module;
        llvm::Function *func;
        llvm::Type *llvmVoidPtrType;
        llvm::IRBuilder<> builder;
        llvm::Value *lastValue;
        llvm::BasicBlock *block, *funcBlock;
        static int argc;
        static char **argv;

        void narrow(model::TypeDef *curType, model::TypeDef *ancestor);

        void setModFunc(model::FuncDef *funcDef, llvm::Function *func) {
            moduleFuncs[funcDef] = func;
        }

        llvm::Function *getModFunc(model::FuncDef *funcDef);

        void setModVar(model::VarDefImpl *varDef, llvm::GlobalVariable *var) {
            moduleVars[varDef] = var;
        }

        llvm::GlobalVariable *getModVar(model::VarDefImpl *varDef);
        
        model::TypeDef *getFuncType(model::Context &context,
                                    const llvm::Type *llvmFuncType
                                    );
        BHeapVarDefImplPtr createLocalVar(BTypeDef *tp, llvm::Value *&var);
        
        BTypeDefPtr createClass(model::Context &context,
                                const std::string &name,
                                unsigned int nextVTableSlot
                                );

        LLVMBuilder();

        virtual BuilderPtr createChildBuilder();

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
                                        const char* fLabel=0);
        
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
                           model::Expr *cond);

        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  );

        virtual void emitBreak(model::Context &context, 
                               model::Branchpoint *branch
                               );

        virtual void emitContinue(model::Context &context, 
                                  model::Branchpoint *branch
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
                             void *cfunc
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

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name,
                                                 bool emitDebugInfo = false
                                                 );
        virtual void closeModule(model::Context &context,
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

        virtual void registerPrimFuncs(model::Context &context);
        virtual void loadSharedLibrary(const std::string &name,
                                       const std::vector<std::string> &symbols,
                                       model::Context &context
                                       );
        virtual void registerImport(model::Context &context, 
                                    model::VarDef *varDef
                                    );

        virtual void setArgv(int argc, char **argv);        
        virtual void run();
        virtual void dump();
        
        // internal functions used by our VarDefImpl to generate the 
        // appropriate variable references.
        void emitMemVarRef(model::Context &context, llvm::Value *val);
        void emitArgVarRef(model::Context &context, llvm::Value *val);
        
        // XXX hack to emit all vtable initializers until we get constructor 
        // composition.
        virtual void emitVTableInit(model::Context &context,
                                    model::TypeDef *typeDef
                                    );
};

} // namespace builder::mvll
} // namespace builder
#endif

