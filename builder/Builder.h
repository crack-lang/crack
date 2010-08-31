// Copyright 2009 Google Inc.

#ifndef _builder_Builder_h_
#define _builder_Builder_h_

#include <spug/RCPtr.h>

#include "model/FuncCall.h" // for FuncCall::ExprVec
#include "model/FuncDef.h" // for FuncDef::Flags

namespace model {
    class AllocExpr;
    class AssignExpr;
    class Branchpoint;
    SPUG_RCPTR(ArgDef);
    SPUG_RCPTR(ModuleDef);
    SPUG_RCPTR(CleanupFrame);
    SPUG_RCPTR(Branchpoint);
    class Context;
    SPUG_RCPTR(FuncCall);
    SPUG_RCPTR(IntConst);
    SPUG_RCPTR(FloatConst);
    class NullConst;
    SPUG_RCPTR(StrConst);
    SPUG_RCPTR(TypeDef);
    SPUG_RCPTR(VarDef);
    SPUG_RCPTR(VarRef);
    class FuncCall;
};

namespace builder {

SPUG_RCPTR(Builder);

/** Abstract base class for builders.  Builders generate code. */
class Builder : public spug::RCBase {

    protected:
        int optimizeLevel;

    public:
        Builder(): optimizeLevel(0) { }

        /**
         * This gets called on the "root builder" everytime a new module gets 
         * loaded and the new builder .  Derived classes may either create a 
         * new Builder instance or return the existing one.
         */
        virtual BuilderPtr createChildBuilder() = 0;

        virtual model::ResultExprPtr emitFuncCall(
            model::Context &context,
            model::FuncCall *funcCall
        ) = 0;
        
        virtual model::ResultExprPtr emitStrConst(model::Context &context,
                                                  model::StrConst *strConst
                                                  ) = 0;

        virtual model::ResultExprPtr emitIntConst(model::Context &context,
                                                  model::IntConst *val
                                                  ) = 0;
        virtual model::ResultExprPtr emitFloatConst(model::Context &context,
                                                    model::FloatConst *val
                                                   ) = 0;

        /**
         * Emits a null of the specified type.
         */        
        virtual model::ResultExprPtr emitNull(model::Context &context,
                                              model::NullConst *nullExpr
                                              ) = 0;
        
        /**
         * Emit an allocator for the specified type.
         */
        virtual model::ResultExprPtr emitAlloc(model::Context &context,
                                               model::AllocExpr *allocExpr,
                                               model::Expr *countExpr = 0
                                               ) = 0;
        
        /**
         * Emit a test for non-zero.  This is the default for emitting 
         * conditionals expressions.
         */
        virtual void emitTest(model::Context &context,
                              model::Expr *expr
                              ) = 0;

        /**
         * Emit the beginning of an "if" statement, returns a Branchpoint that 
         * must be passed to the subsequent emitElse() or emitEndIf().
         */
        virtual model::BranchpointPtr emitIf(model::Context &context,
                                             model::Expr *cond) = 0;
        
        /**
         * Emits an "else" statement.
         * @params pos the branchpoint returned from the original emitIf().
         * @param terminal true if the "if" clause was terminal.
         * @returns a branchpoint to be passed to the subsequent emitEndIf().
         */
        virtual model::BranchpointPtr
            emitElse(model::Context &context,
                     model::Branchpoint *pos,
                     bool terminal
                     ) = 0;
        
        /**
         * Closes off an "if" statement emitted by emitIf().
         * @param pos a branchpoint returned from the last emitIf() or 
         *  emitElse().
         * @param terminal true if the last clause (if or else) was terminal.
         */
        virtual void emitEndIf(model::Context &context,
                               model::Branchpoint *pos,
                               bool terminal
                               ) = 0;
        
        /**
         * Emits a "while" statement.
         * @param cond the conditional expression.
         * @returns a Branchpoint to be passed into the emitEndWhile()
         */
        virtual model::BranchpointPtr 
            emitBeginWhile(model::Context &context, 
                           model::Expr *cond
                           ) = 0;

        /**
         * Emits the end of the "while" statement.
         * @param pos the branchpoint object returned from the emitWhile().
         */        
        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos,
                                  bool isTerminal
                                  ) = 0;

        /**
         * Emit the code for a break statement (branch to the end of the 
         * enclosing while/for/switch).
         * @param branch branchpoint for the loop we're breaking out of.
         */
        virtual void emitBreak(model::Context &context,
                               model::Branchpoint *branch
                               ) = 0;
        
        /**
         * Emit the code for the continue statement (branch to the beginning 
         * of the enclosing while/for).
         * @param branch branchpoint for the loop we're breaking out of.
         */
        virtual void emitContinue(model::Context &context,
                                  model::Branchpoint *branch
                                  ) = 0;

        /**
         * Returns a forward definition for a function.
         */
        virtual model::FuncDefPtr
            createFuncForward(model::Context &context,
                              model::FuncDef::Flags flags,
                              const std::string &name,
                              model::TypeDef *returnType,
                              const std::vector<model::ArgDefPtr> &args,
                              model::FuncDef *override
                              ) = 0;
        
        /**
         * Create a forward defintion for a class.
         */
        virtual model::TypeDefPtr
            createClassForward(model::Context &context,
                               const std::string &name
                               ) = 0;

        /**
         * Start a new function definition.
         * @param args the function argument list.
         * @param existing either the virtual base function that we are 
         * overriding or the forward declaration that we are implementing, 
         * null if this is not an override or forward-declared.
         */
        virtual model::FuncDefPtr
            emitBeginFunc(model::Context &context,
                          model::FuncDef::Flags flags,
                          const std::string &name,
                          model::TypeDef *returnType,
                          const std::vector<model::ArgDefPtr> &args,
                          model::FuncDef *existing
                          ) = 0;
        
        /**
         * Emit the end of a function definition.
         */
        virtual void emitEndFunc(model::Context &context,
                                 model::FuncDef *funcDef) = 0;
        
        /**
         * Create an external primitive function reference.
         */
        virtual model::FuncDefPtr
            createExternFunc(model::Context &context,
                             model::FuncDef::Flags flags,
                             const std::string &name,
                             model::TypeDef *returnType,
                             const std::vector<model::ArgDefPtr> &args,
                             void *cfunc
                             ) = 0;
        
        /**
         * Emit the beginning of a class definition.
         * The context should be the context of the new class.
         */
        virtual model::TypeDefPtr
            emitBeginClass(model::Context &context,
                           const std::string &name,
                           const std::vector<model::TypeDefPtr> &bases,
                           model::TypeDef *forwardDef
                           ) = 0;

        /**
         * Emit the end of a class definitiion.
         * The context should be the context of the class.
         */
        virtual void emitEndClass(model::Context &context) = 0;

        /**
         * Emit a return statement.
         * @params expr an expression or null if we are returning void.
         */
        virtual void emitReturn(model::Context &context,
                                model::Expr *expr) = 0;

        /**
         * Emits a variable definition and returns a new VarDef object for the 
         * variable.
         * @param staticScope true if the "static" keyword was applied to the 
         *        definition.
         */
        virtual model::VarDefPtr emitVarDef(
            model::Context &container,
            model::TypeDef *type,
            const std::string &name,
            model::Expr *initializer = 0,
            bool staticScope = false
        ) = 0;
    
        virtual model::ArgDefPtr createArgDef(model::TypeDef *type,
                                              const std::string &name
                                              ) = 0;

        
        /**
         * @param squashVirtual If true, call a virtualized function 
         *  directly, without the use of the vtable (causes virtualized to be 
         *  set to false, regardless of whether funcDef is virtual).
         */
        virtual model::FuncCallPtr 
            createFuncCall(model::FuncDef *func, 
                           bool squashVirtual = false
                           ) = 0;

        virtual model::VarRefPtr
            createVarRef(model::VarDef *varDef) = 0;
        
        /**
         * Create a field references - field references obtain the value of a 
         * class instance variable, so the type of the returned expression 
         * will be the same as the type of the varDef.
         */
        virtual model::VarRefPtr
            createFieldRef(model::Expr *aggregate,
                           model::VarDef *varDef
                           ) = 0;
        
        /**
         * Create a field assignment.
         * @param aggregate the aggregate containing the field.
         * @param assign the assignment expression.
         */
        virtual model::ResultExprPtr emitFieldAssign(model::Context &context,
                                                     model::Expr *aggregate,
                                                     model::AssignExpr *assign
                                                     ) = 0;

        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name,
                                                 bool emitDebugInfo = false
                                                 ) = 0;
        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *modDef
                                 ) = 0;
        
        /**
         * Create a new cleanup frame.
         */
        virtual model::CleanupFramePtr
            createCleanupFrame(model::Context &context) = 0;
        
        /**
         * Close all of the cleanup frames.
         * This is a signal to the builder to emit all of the cleanup code
         * for the context.
         */
        virtual void closeAllCleanups(model::Context &context) = 0;
        
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val) = 0;
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val,
                                                  model::TypeDef *type = 0
                                                  ) = 0;
        virtual model::FloatConstPtr createFloatConst(model::Context &context,
                                                    double val,
                                                    model::TypeDef *type = 0
                                                    ) = 0;
        
        virtual void registerPrimFuncs(model::Context &context) = 0;

        /**
         * Load the named shared library, store the addresses for the symbols 
         * as StubDef's in 'context'.
         */
        virtual void loadSharedLibrary(const std::string &name,
                                       const std::vector<std::string> &symbols,
                                       model::Context &context
                                       ) = 0;
        
        /**
         * This is called for every symbol that is imported into a module.  
         * Implementations should do whatever processing is necessary.
         */
        virtual void registerImport(model::Context &context, 
                                    model::VarDef *varDef
                                    ) = 0;

        /**
         * Provides the builder with access to the program's argument list.
         */
        virtual void setArgv(int argc, char **argv) = 0;
        
        virtual void run() = 0;
        
        /// Dump the compiled op-codes to standard output.
        virtual void dump() = 0;

        // XXX hack to emit all vtable initializers until we get constructor 
        // composition.
        virtual void emitVTableInit(model::Context &context,
                                    model::TypeDef *typeDef
                                    ) = 0;

        // implementation specific optimization level
        void setOptimize(int level) { optimizeLevel = level; }

};

} // namespace builder

#endif

