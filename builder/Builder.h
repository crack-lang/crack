// Copyright 2009-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _builder_Builder_h_
#define _builder_Builder_h_

#include <stdint.h>
#include <spug/RCPtr.h>

#include "BuilderOptions.h"
#include "model/FuncCall.h" // for FuncCall::ExprVec
#include "model/FuncDef.h" // for FuncDef::Flags
#include "model/ImportedDef.h"

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
    SPUG_RCPTR(TernaryExpr);
    SPUG_RCPTR(TypeDef);
    SPUG_RCPTR(VarDef);
    SPUG_RCPTR(VarRef);
    class FuncCall;
};

namespace builder {

SPUG_RCPTR(Builder);

/** Abstract base class for builders.  Builders generate code. */
class Builder : public spug::RCBase {

    public:
        /** 
         * Abstract representation of an opened cache file. Specialized by 
         * builder implementations.
         */
        class CacheFile : public RCBase {};
        SPUG_RCPTR(CacheFile);

        BuilderOptionsPtr options;

        Builder(): options(0) { }

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
         * Create a ternary expression object.
         */
        virtual model::TernaryExprPtr createTernary(model::Context &context,
                                                    model::Expr *cond,
                                                    model::Expr *trueVal,
                                                    model::Expr *falseVal,
                                                    model::TypeDef *type
                                                    ) = 0;
        
        /**
         * Emit a ternary operator expression.
         * @param cond the conditional expression.
         * @param expr the expression to emit.
         */
        virtual model::ResultExprPtr emitTernary(model::Context &context,
                                                 model::TernaryExpr *expr
                                                 ) = 0;

        /**
         * Emits a "while" statement.
         * @param cond the conditional expression.
         * @returns a Branchpoint to be passed into the emitEndWhile()
         */
        virtual model::BranchpointPtr 
            emitBeginWhile(model::Context &context, 
                           model::Expr *cond,
                           bool gotPostLoop
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
         * Emit code to be called at the end of the while loop.
         * All code emitted after this will be called after the completion of 
         * the body of the loop regardless of whether a continue statement was 
         * invoked.
         * This must be called between emitBeginWhile() and emitEndWhile().  
         * The "gotPostLoop" argument to the emitBeginWhile() must be true.
         */
        virtual void emitPostLoop(model::Context &context,
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
                             model::TypeDef *receiverType,
                             const std::vector<model::ArgDefPtr> &args,
                             void *cfunc,
                             const char *symbolName=0
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
         * Emit the beginning of a try block.  Try/catch statements are 
         * roughly emitted as:
         *  bpos = emitBeginTry()
         *  emitCatch(bpos)
         *  emitCatch(bpos)
         *  emitEndTry(bpos);
         */
        virtual model::BranchpointPtr emitBeginTry(model::Context &context
                                                   ) = 0;
        
        /**
         * Emit a catch clause.
         * 'context' should be a context that was marked as a catch 
         * context using setCatchBranchpoint() for the code between 
         * emitBeginTry() and the first emitCatch().
         * @param terminal true if the last catch block (or try block for the 
         *  first catch) is terminal.
         * @returns an expression that can be used to initialize the exception 
         *  variable.
         */
        virtual model::ExprPtr emitCatch(model::Context &context,
                                         model::Branchpoint *branchpoint,
                                         model::TypeDef *catchType,
                                         bool terminal
                                         ) = 0;

        /**
         * Close off an existing try block.
         * The rules for 'context' in emitCatch() apply.
         * @param terminal true if the last catch block (or try block for the 
         *  first catch) is terminal.
         */
        virtual void emitEndTry(model::Context &context,
                                model::Branchpoint *branchpoint,
                                bool terminal
                                ) = 0;
    
        /**
         * Called in a catch block to give the builder the opportunity to add 
         * an exception cleanup to the cleanup frame for the block.  Builders 
         * can use this to cleanup whatever special housekeeping data they 
         * need for the exception.
         */
        virtual void emitExceptionCleanup(model::Context &context) = 0;
        
        /** Emit an exception "throw" */
        virtual void emitThrow(model::Context &context,
                               model::Expr *expr
                               ) = 0;

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
        
        /**
         * Creates an "offset field" - this is a special kind of instance 
         * variable that resised at a specific offset from the instance base 
         * pointer.  These are used for mapping instance variables in 
         * extension objects.
         */
        virtual model::VarDefPtr createOffsetField(model::Context &context,
                                                   model::TypeDef *type,
                                                   const std::string &name,
                                                   size_t offset
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

        /**
         * Create a new module.
         * @param context the module context.
         * @param name the module's canonical name.
         * @param path the full path to the existing source on disk
         * @param owner the module's owner - this should be null unless the
         *  module is being defined inside another module that it depends on.
         */
        virtual model::ModuleDefPtr createModule(model::Context &context,
                                                 const std::string &name,
                                                 const std::string &path,
                                                 model::ModuleDef *owner
                                                 ) = 0;

        virtual void closeModule(model::Context &context,
                                 model::ModuleDef *modDef
                                 ) = 0;

        /**
         * Open the cache file(s) to be used by materializeModule, or return 
         * null if unable to.
         * 
         * Separating this out into two methods facilitates the file locking 
         * mechanism, see model/Context.cc:cacheModule().
         * @param context the module context.
         * @param canonicalName the module's canonical name.
         */
        virtual CacheFilePtr getCacheFile(
            model::Context &context,
            const std::string &canonicalName
        ) = 0;

        /**
         * Materialize a module from a builder specific cache. Returns NULL in
         * the event of a cache miss.
         * @param context the module context.
         * @param cacheFile cache file object returned from getCacheFile().
         * @param canonicalName the module's canonical name.
         * @param owner the module's owner - this should be null unless the
         *  module is being defined inside another module that it depends on.
         * @return a module that has been loaded from the cache, or null if 
         *  unavailable
         */
        virtual model::ModuleDefPtr materializeModule(
            model::Context &context,
            CacheFile *cacheFile,
            const std::string &canonicalName,
            model::ModuleDef *owner
        ) = 0;

        /**
         * Create a new VarDef from the current (cached) module.  The 
         * definition must already exist in the underlying representation of 
         * the module.
         */
        virtual model::VarDefPtr materializeVar(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type,
            int instSlot
        ) = 0;

        /**
         * Create a new ArgDef from the current (cached) module.
         */
        virtual model::ArgDefPtr materializeArg(
            model::Context &context,
            const std::string &name,
            model::TypeDef *type
        ) = 0;

        /**
         * Create a new TypeDef from the current (cached) module.
         * See materializeVar(), same rules apply.
         */        
        virtual model::TypeDefPtr materializeType(
            model::Context &context,
            const std::string &name,
            const std::string &namespaceName
        ) = 0;

        /**
         * Create a new FuncDef from the current (cached) module.
         * See materializeVar(), same rules apply.
         */
        virtual model::FuncDefPtr materializeFunc(
            model::Context &context,
            model::FuncDef::Flags flags,
            const std::string &name,
            model::TypeDef *returnType,
            const model::ArgVec &args
        ) = 0;
        
        /**
         * Write the module implementation to the persistent cache in whatever 
         * format is appropriate.
         * 
         * This should also clean up any other cached copies of the module.
         * 
         * @param uniquifier A string that is globally unique at a given point 
         *  in time (generally hostname.pid) that should be appended to all 
         *  file ids.
         */
        virtual void cacheModule(
            model::Context &context,
            model::ModuleDef *module,
            const std::string &uniquifier
        ) = 0;
        
        /**
         * Convert the uniquified cached module to the non-unique one.
         * This basically moves a file of the form modulename.uniquifier to 
         * modulename after the metadata file has been moved into place.
         * @param retain if true, keep the cache file, otherwise delete it.
         */
        virtual void finishCachedModule(
            model::Context &context,
            model::ModuleDef *module,
            const std::string &uniquifier,
            bool retain
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
                                                  int64_t val,
                                                  model::TypeDef *type = 0
                                                  ) = 0;
        virtual model::IntConstPtr createUIntConst(model::Context &context,
                                                   uint64_t val,
                                                   model::TypeDef *type = 0
                                                   ) = 0;
        virtual model::FloatConstPtr createFloatConst(model::Context &context,
                                                    double val,
                                                    model::TypeDef *type = 0
                                                    ) = 0;
        
        /**
         * register the primitive types and functions into a .builtin module,
         * which is returned.
         */
        virtual model::ModuleDefPtr registerPrimFuncs(model::Context &context
                                                      ) = 0;

        /**
         * Initialize a root builder with the root context.  This will 
         * be called after the root context is created.
         */
        virtual void initialize(model::Context &context) = 0;

        /**
         * Load the named shared library, returning a handle suitable for
         * retrieving symbols from the library using the local mechanism
         */
        virtual void *loadSharedLibrary(const std::string &name) = 0;

        /**
         * Load the named shared library and create stub definitions for the 
         * specified symbols.
         * If 'ns' is not null, add an alias for each of the symbols in the 
         * namespace.
         */
        virtual void importSharedLibrary(const std::string &name,
                                         const model::ImportedDefVec &symbols,
                                         model::Context &context,
                                         model::Namespace *ns
                                         ) = 0;

        /**
         * This is called for every symbol that is imported into a module.  
         * Implementations should do whatever processing is necessary.
         */
        virtual void registerImportedDef(model::Context &context,
                                         model::VarDef *varDef
                                         ) = 0;

        /**
         * This is called once per imported module, to allow the builder
         * to emit any required initialization instructions for the imported
         * module, i.e. to emit a call to run its top level code
         */
        virtual void initializeImport(model::ModuleDef*,
                                      const model::ImportedDefVec &symbols
                                      ) = 0;

        /**
         * Provides the builder with access to the program's argument list.
         */
        virtual void setArgv(int argc, char **argv) = 0;

        /**
         * Called after all modules have been parsed/run. Only called
         * on root builder, not children
         */
        virtual void finishBuild(model::Context &context) = 0;

        /**
         * If a builder can directly execute functions from modules it builds,
         * e.g. via JIT, then this will return true
         */
        virtual bool isExec() = 0;

        // XXX hack to emit all vtable initializers until we get constructor 
        // composition.
        virtual void emitVTableInit(model::Context &context,
                                    model::TypeDef *typeDef
                                    ) = 0;

};

} // namespace builder

#endif

