// Copyright 2009 Google Inc.

#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>
#include "Construct.h"
#include "FuncDef.h"
#include "parser/Location.h"

namespace builder {
    class Builder;
}

namespace parser {
    class Token;
}

namespace model {

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
        // break and continue branchpoints
        BranchpointPtr breakBranch, continueBranch;
        
        // the current source location.
        parser::Location loc;
        
        // initializer for an empty location object
        static parser::Location emptyLoc;
        
        // emit a variable definition with no error checking.
        VarDefPtr emitVarDef(Context *defCtx, TypeDef *type,
                             const std::string &name,
                             Expr *initializer
                             );

        // issue a warning if defining the variable would hide a symbol in an
        // enclosing context.
        void warnOnHide(const std::string &name);

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
        
        // this is the return type for a function context, and the class type 
        // for a class context.
        TypeDefPtr returnType;
        
        // the current cleanup frame.
        CleanupFramePtr cleanupFrame;
        
        // flags to be injected into the next function
        FuncDef::Flags nextFuncFlags;

        // the construct
        Construct *construct;
    
        Context(builder::Builder &builder, Scope scope, Context *parentContext,
                Namespace *ns,
                Namespace *compileNS
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
        ContextPtr createSubContext(Scope newScope, Namespace *ns = 0);

        /**
         * Create a new subcontext in the same scope.
         */
        ContextPtr createSubContext() {
            return createSubContext(scope, 0);
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
         * Returns the parent of the context.
         */
        ContextPtr getParent() {
            return parent;
        }

        /**
         * Returns true if the context encloses the "other" context - a 
         * context encloses another context if it is an ancestor of the other 
         * context.
         */
        bool encloses(const Context &other) const;

        ModuleDefPtr createModule(const std::string &name);

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
        void emitVarDef(TypeDef *type, const parser::Token &name, 
                        Expr *initializer
                        );

        /**
         * Create a ternary expression.
         */
        ExprPtr createTernary(Expr *cond, Expr *trueVal, Expr *falseVal);

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
         * Create a reference to the "this" variable, error if there is none.
         */
        model::ExprPtr makeThisRef(const std::string &memberName);

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
        
        /**
         * Look up the annotation in the compile namespace.  Returns null if 
         * undefined.
         */
        AnnotationPtr lookUpAnnotation(const std::string &name);

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
        
        void dump(std::ostream &out, const std::string &prefix) const;
        void dump();
};

inline std::ostream &operator <<(std::ostream &out, const Context &context) {
    context.dump(out, "");
    return out;
}

}; // namespace model

#endif


