
#ifndef _builder_Builder_h_
#define _builder_Builder_h_

#include <spug/RCPtr.h>

#include "model/FuncCall.h" // for FuncCall::ExprVec
#include "model/FuncDef.h" // for FuncDef::Flags

namespace model {
    SPUG_RCPTR(ArgDef);
    SPUG_RCPTR(Branchpoint);
    class Context;
    class IntConst;
    SPUG_RCPTR(FuncCall);
    SPUG_RCPTR(IntConst);
    SPUG_RCPTR(StrConst);
    SPUG_RCPTR(TypeDef);
    SPUG_RCPTR(VarDef);
    SPUG_RCPTR(VarRef);
    class FuncCall;
};

namespace builder {

/** Abstract base class for builders.  Builders generate code. */
class Builder {
    public:
        virtual void emitFuncCall(model::Context &context,
                                  model::FuncDef *func,
                                  model::Expr *receiver,
                                  const model::FuncCall::ExprVec &args
                                  ) = 0;
        
        virtual void emitStrConst(model::Context &context,
                                  model::StrConst *strConst
                                  ) = 0;

        virtual void emitIntConst(model::Context &context,
                                  const model::IntConst &val) = 0;
        
        /**
         * Emit an allocator for the specified type.
         */
        virtual void emitAlloc(model::Context &context,
                               model::TypeDef *type
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
                           model::Expr *cond) = 0;

        /**
         * Emits the end of the "while" statement.
         * @param pos the branchpoint object returned from the emitWhile().
         */        
        virtual void emitEndWhile(model::Context &context,
                                  model::Branchpoint *pos) = 0;

        /**
         * Start a new function definition.
         * @param args the function argument list.
         */
        virtual model::FuncDefPtr
            emitBeginFunc(model::Context &context,
                          model::FuncDef::Flags flags,
                          const std::string &name,
                          model::TypeDef *returnType,
                          const std::vector<model::ArgDefPtr> &args) = 0;
        
        /**
         * Emit the end of a function definition.
         */
        virtual void emitEndFunc(model::Context &context,
                                 model::FuncDef *funcDef) = 0;
        
        /**
         * Emit the beginning of a class definition.
         * The context should be the context of the new class.
         */
        virtual model::TypeDefPtr
            emitBeginClass(model::Context &context,
                           const std::string &name,
                           const std::vector<model::TypeDefPtr> &bases) = 0;

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
        virtual model::FuncCallPtr 
            createFuncCall(model::FuncDef *func) = 0;
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
         * @param varDef the field variable definition.
         * @param val the value to assign to the variable.
         */
        virtual void emitFieldAssign(model::Context &context,
                                     model::Expr *aggregate,
                                     model::VarDef *varDef,
                                     model::Expr *val
                                     ) = 0;

        /**
         * Emit code to narrow an instance of curType to parent.
         * You only need to implement a working version of this if your 
         * builder calls TypeDef::emitNarrower().
         * @param curType the type that you currently have.
         * @param parent the type that you want to end up with.
         * @param index the base class index (the index of 'parent' within the 
         *  base classes of 'curType')
         */
        virtual void emitNarrower(model::TypeDef &curType,
                                  model::TypeDef &parent,
                                  int index
                                  ) = 0;

        virtual void createModule(const char *name) = 0;
        virtual void closeModule() = 0;
        virtual model::StrConstPtr createStrConst(model::Context &context,
                                                  const std::string &val) = 0;
        virtual model::IntConstPtr createIntConst(model::Context &context,
                                                  long val) = 0;
        
        virtual void registerPrimFuncs(model::Context &context) = 0;
        
        virtual void run() = 0;
        
        /// Dump the compiled op-codes to standard output.
        virtual void dump() = 0;
};

} // namespace builder

#endif

