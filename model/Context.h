// Copyright 2009-2012 Google Inc.
// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>
#include "Construct.h"
#include "FuncDef.h"
#include "Import.h"
#include "ImportedDef.h"
#include "parser/Location.h"

namespace builder {
    SPUG_RCPTR(Builder);
}

namespace parser {
    class Token;
}

namespace model {

class Construct;

SPUG_RCPTR(Annotation);
SPUG_RCPTR(Branchpoint);
SPUG_RCPTR(BuilderContextData);
SPUG_RCPTR(CleanupFrame);
SPUG_RCPTR(Expr);
SPUG_RCPTR(ModuleDef);
SPUG_RCPTR(Namespace);
SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDef);
SPUG_RCPTR(VarRef);

SPUG_RCPTR(Context);

/**
 * Holds everything relevant to the current parse context.
 */
class Context : public spug::RCBase {
    private:
        // break, continue and catch branchpoints
        BranchpointPtr breakBranch, continueBranch, catchBranch;
        
        // the current source location.
        parser::Location loc;

        // @import statements.                
        std::vector<ImportPtr> compileNSImports;
        
        // initializer for an empty location object
        static parser::Location emptyLoc;
        
        // emit a variable definition with no error checking.
        VarDefPtr emitVarDef(Context *defCtx, TypeDef *type,
                             const std::string &name,
                             Expr *initializer,
                             bool constant = false
                             );

        // issue a warning if defining the variable would hide a symbol in an
        // enclosing context.
        void warnOnHide(const std::string &name);

        // create a new overload for srcNs that correctly delegates to the 
        // ancestor namespace.
        OverloadDefPtr replicateOverload(const std::string &varName,
                                         Namespace *srcNs
                                         );

        /**
         * this uses Location to show the
         * source line and caret position of the problem
         */
        void showSourceLoc(const parser::Location &loc, std::ostream &out);

    public:

        // context scope - this is used to control how variables defined in 
        // the scope are stored.
        enum Scope {
            module,
            instance,
            local,
            composite  // scope is just a composition of parent scopes
        };
        
        ContextPtr parent;
        
        // the context namespace.
        NamespacePtr ns;
        
        // the compile namespace (used to resolve annotations).
        NamespacePtr compileNS;

        builder::Builder &builder;
        BuilderContextDataPtr builderData;
        Scope scope;
        
        // true if the context is the outermost context of a function.
        bool toplevel;
        
        // true if we are currently in the process of emitting cleanups - 
        // prevents us from trying to add cleanups on the expressions that we 
        // are cleaning up.
        bool emittingCleanups;
        
        // if true, a terminal statement has been emitted in the context.
        bool terminal;

        // set to true for the module context of a generic.        
        bool generic;
        
        // this is the return type for a function context, and the class type 
        // for a class context.
        TypeDefPtr returnType;

        // the current cleanup frame.
        CleanupFramePtr cleanupFrame;
        
        // flags to be injected into the next function
        FuncDef::Flags nextFuncFlags;
        
        // flags to be injected into the next class
        TypeDef::Flags nextClassFlags;

        // vtable offset if this is a function context
        unsigned int vtableOffset;

        // the construct
        Construct *construct;
    
        Context(builder::Builder &builder, Scope scope, Context *parentContext,
                Namespace *ns,
                Namespace *compileNS = 0
                );
        
        Context(builder::Builder &builder, Scope scope, Construct *construct,
                Namespace *ns,
                Namespace *compileNS
                );
        
        ~Context();
        
        /**
         * Create a new subcontext with a different scope from the parent 
         * context.
         */
        ContextPtr createSubContext(Scope newScope, Namespace *ns = 0,
                                    const std::string *name = 0,
                                    Namespace *cns = 0
                                    );

        /**
         * Create a new subcontext in the same scope.
         * @param sameCNS if true, use the compile namespace of the existing 
         *  context.
         */
        ContextPtr createSubContext(bool sameCNS = false) {
            return createSubContext(scope, 0, 0, 
                                    sameCNS ? compileNS.get() : 0
                                    );
        }

        /**
         * Returns the depth-first closest enclosing class context, null if 
         * there is none.  (class contexts are contexts with scope = instance)
         */
        ContextPtr getClassContext();
        
        /**
         * Returns the depth-first closest enclosing definition context, 
         * raises an exception if there is none.
         * Definition contexts are non-composite contexts.
         */
        ContextPtr getDefContext();
        
        /**
         * Returns the depth-first closest enclosing toplevel context.
         */
        ContextPtr getToplevel();
        
        /**
         * Returns the first enclosing module context.
         */
        ContextPtr getModuleContext();
        
        /**
         * Returns the parent of the context.
         */
        ContextPtr getParent() {
            return parent;
        }
        
        /** Returns true if this a generic context (directly or indirecly). */
        bool isGeneric() {
            return getModuleContext()->generic;
        }

        /**
         * Returns the compile-time construct - this will revert to the 
         * default construct if there is no compile-time construct.
         */
        Construct *getCompileTimeConstruct();

        /**
         * Returns true if the context encloses the "other" context - a 
         * context encloses another context if it is an ancestor of the other 
         * context.
         */
        bool encloses(const Context &other) const;

        ModuleDefPtr createModule(const std::string &name,
                                  const std::string &path = "",
                                  ModuleDef *owner = 0
                                  );

        /**
         * Try to load the module from the cache, return true if the module 
         * exists in the cache and is up to date.
         */
        ModuleDefPtr materializeModule(const std::string &canonicalName,
                                       ModuleDef *owner = 0
                                       );

        /**
         * Store the module in the cache.
         */
        void cacheModule(ModuleDef *mod);

        /** 
         * Get or create a string constant.  This can be either a
         * "StaticString(StrConst, uint size)" expression if StaticString is 
         * defined, or a simple StrConst if it is not.
         * @param raw if true, create a byteptr even if StaticString is 
         *  defined.
         */
        ExprPtr getStrConst(const std::string &value, bool raw = false);

        /**
         * Create a new cleanup frame.  Cleanup frames group all 
         * cleanups that are emitted until the frame is closed with 
         * closeCleanupFrame().  Code emitted within a cleanup frame must 
         * call all of the cleanups whenever exiting from the scope of the 
         * frame, no matter how it exits.
         */
        CleanupFramePtr createCleanupFrame();
        
        /**
         * Closes the current cleanup frame, emitting all cleaup code if 
         * appropriate.
         */
        void closeCleanupFrame();

        /**
         * Create a forward declaration for the class and return it.
         */
        TypeDefPtr createForwardClass(const std::string &name);

        /**
         * Checks for any forward declarations that have not been defined.
         * Throws a ParseError if there are any.
         */
        void checkForUnresolvedForwards();
        
        /**
         * Emit a variable definition in the context.
         */
        VarDefPtr emitVarDef(TypeDef *type, const parser::Token &name, 
                             Expr *initializer,
                             bool constant = false
                             );

        /**
         * Create a ternary expression.
         */
        ExprPtr createTernary(Expr *cond, Expr *trueVal, Expr *falseVal);

        /**
         * Emit a sequence initializer.
         */
        ExprPtr emitConstSequence(TypeDef *type, 
                                  const std::vector<ExprPtr> &elems
                                  );

        /** 
         * Emit an import statement.   Returns the imported module.
         * 
         * @param ns The namespace to populate
         * @param moduleName This is normally the module name, split up into 
         *     segments.  If 'rawSharedLib' is true, it contains the shared 
         *     library path as a single string.
         * @param imports The list of imported symbols.
         * @param annotation If true, the import is called from within 
         *     annotation context.
         * @param recordImport If true, and 'annotation' is also true, record 
         *     the import in the parent context (this has a very specific use 
         *     case, it's for when we do an import statement in the context 
         *     of an annotation, in which case the context is a temporary 
         *     annotation context and the parent context needs to retain 
         *     record of all compile namespace imports).
         * @param rawSharedLib If true, this is the import of a raw shared 
         *     library.
         * @param symLocs If present, this is the locations of the symbols in 
         *     the source file.  This will allow the function to produce 
         *     better error messages.
         */
        ModuleDefPtr emitImport(Namespace *ns, 
                                const std::vector<std::string> &moduleName,
                                const ImportedDefVec &imports,
                                bool annotation,
                                bool recordImport,
                                bool rawSharedLib,
                                std::vector<parser::Location> *symLocs = 0
                                );

        /**
         * Returns true if the namespace is in the same function as the 
         * context.
         */
        bool inSameFunc(Namespace *varNS);

        /**
         * Create a variable reference from the context and check that the 
         * variable is actually reachable from the context.
         */
        ExprPtr createVarRef(VarDef *def);
        
        /**
         * Create a field reference and check that the variable is actually in 
         * the aggregate.
         */
        VarRefPtr createFieldRef(Expr *aggregate, VarDef *var);

        /**
         * Set the branchpoint to be used for a break statement.
         * @param branch the branchpoint, may be null.
         */
        void setBreak(Branchpoint *branch);
        
        /**
         * Set the branchpoint to be used for a continue statement.
         * @param branch the branchpoint, may be null.
         */
        void setContinue(Branchpoint *branch);
        
        /**
         * Set the "catch" flag, indicating that this context is in a try 
         * block.
         */
        void setCatchBranchpoint(Branchpoint *branch);
        
        /**
         * Obtains the branchpoint to be used for a break statement, returns 
         * null if there is none.
         */
        Branchpoint *getBreak();
        
        /**
         * Obtains the branchpoint to be used for a continue statement, 
         * returns null if there is none.
         */
        Branchpoint *getContinue();
        
        /**
         * Returns the catch context - this is either the first enclosing 
         * context with a try/catch statement or the parent of the toplevel 
         * context.
         */
        ContextPtr getCatch();
        
        /**
         * Returns the catch branchpoint for the context.
         */
        BranchpointPtr getCatchBranchpoint();

        /**
         * Create a reference to the "this" variable, error if there is none.
         */
        model::ExprPtr makeThisRef(const std::string &memberName);

        /**
         * Returns true if the context is in a method of /type/ and thus has 
         * access to instance variables and methods of the type.
         */
        bool hasInstanceOf(TypeDef *type) const;

        /**
         * Expand an iterator style for loop into initialization, condition 
         * and after-body.  The initialization will actually be emitted, 
         * condition and after-body will be filled in.
         */
        void expandIteration(const std::string &name, bool defineVar,
                             bool isIter, 
                             Expr *seqExpr,
                             ExprPtr &cond,
                             ExprPtr &beforeBody,
                             ExprPtr &afterBody
                             );

        // Function store/lookup methods (these are handled through a Context 
        // instead of a namespace because Context can better do overload 
        // management)

        /**
         * Looks up a symbol in the context.  Use this when looking up an 
         * overload definition if you care about it including all possible 
         * overloads accessible from the scope.
         */
        VarDefPtr lookUp(const std::string &varName, Namespace *srcNs = 0);
    
        /**
         * Looks up a function matching the given expression list.
         * 
         * @param context the current context (distinct from the lookup 
         *  context)
         * @param varName the function name
         * @param vals list of parameter expressions.  These will be converted 
         *  to conversion expressions of the correct type for a match.
         * @param srcNs if specified, the namespace to do the lookup in 
         *  (default is the context's namespace)
         * @param allowOverrides by default, this function will ignore 
         *  overrides of a virtual function in considering a match - this is 
         *  to provide the correct method resolution order.  Setting this flag 
         *  to true causes it to return the first matching function, 
         *  regardless of whether it is an override.
         */
        FuncDefPtr lookUp(const std::string &varName,
                          std::vector<ExprPtr> &vals,
                          Namespace *srcNs = 0,
                          bool allowOverrides = false
                          );
        
        /**
         * Look up a function with no arguments.  This is provided as a 
         * convenience, as in this case we don't need to pass the call context.
         * @param acceptAlias if false, ignore an alias.
         * @param srcNs if specified, the namespace to do the lookup in 
         *  (default is the context's namespace)
         */
        FuncDefPtr lookUpNoArgs(const std::string &varName, 
                                bool acceptAlias = true,
                                Namespace *srcNs = 0
                                );

        /**
         * Add a new variable definition to the context namespace.  This is 
         * preferable to the Namespace::addDef() method in that it wraps 
         * FuncDef objects in an OverloadDef.
         * @returns the definition that was actually stored.  This could be 
         *  varDef or 
         */
        VarDefPtr addDef(VarDef *varDef, Namespace *srcNs = 0);

        /**
         * Insures that if there is a child overload definition for 'overload' 
         * in the context's namespace, that it delegates to 'overload' in 
         * 'ancestor's namespace.
         */
        void insureOverloadPath(Context *ancestor, OverloadDef *overload);

        /**
         * If an overload exists for varName,
         * this will write overload possibilities to stream out
         */
        void maybeExplainOverload(std::ostream &out,
                                  const std::string &varName,
                                  Namespace *srcNs);

        // location management

        /**
         * Set the current source location.
         */        
        void setLocation(const parser::Location loc0) {
            loc = loc0;
        }
        
        /**
         * Get the current location.
         */
        const parser::Location &getLocation() const {
            return loc;
        }
        
        // annotation management

        /**
         * Look up the annotation in the compile namespace.  Returns null if 
         * undefined.
         */
        AnnotationPtr lookUpAnnotation(const std::string &name);

        /**
         * Get the compile time imports of this context and all parent 
         * contexts in the order in which they should be applied and add them 
         * to 'imports'.
         */        
        void collectCompileNSImports(std::vector<ImportPtr> &imports) const;

        // error/warning handling

        /**
         * Emit an error message.  If 'throwException' is true, a 
         * ParseException will be thrown.  Otherwise, exit() is called to 
         * terminate the program.
         */
        void error(const parser::Location &loc, const std::string &msg, 
                   bool throwException = true
                   );
        
        /**
         * Emit an error message using the last recorded location.
         */
        void error(const std::string &msg, bool throwException = true) {
            error(loc, msg, throwException);
        }
        
        /**
         * Emit the warning.
         */
        void warn(const parser::Location &loc, const std::string &msg);
        
        /**
         * Emit the warning using the last recorded location.
         */
        void warn(const std::string &msg) {
            warn(loc, msg);
        }
        
        /**
         * Push an error context.  Error contexts will be displayed indented 
         * under an error or warning in the reverse order in which they were 
         * pushed.
         */
        void pushErrorContext(const std::string &msg);
        
        /**
         * Pop and discard the last error context.
         */
        void popErrorContext();
        
        /**
         * Verifies that the variable is accessible within the context (that 
         * it is not some other namespace's private or protected variable).
         * 'name' is the name that the variable is referenced under.  It allows 
         * correct behavior under aliases, which can be used to admit 
         * accessiblity to private names.
         */
        void checkAccessible(VarDef *var, const std::string &name);
        
        /**
         * Used to indicate that somthing in the current context depends on 
         * the module, and as such the module must be loaded and initialized 
         * in order for the current module to work.
         * 
         * This was added to accomodate generics instantiations, which act 
         * like an implicit import of a dependent module.
         */
        void recordDependency(ModuleDef *module);
        
        void dump(std::ostream &out, const std::string &prefix) const;
        void dump();
};

inline std::ostream &operator <<(std::ostream &out, const Context &context) {
    context.dump(out, "");
    return out;
}

}; // namespace model

#endif


