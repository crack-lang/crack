// Copyright 2009 Google Inc.

#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace builder {
    class Builder;
}

namespace parser {
    class Token;
}

namespace model {

SPUG_RCPTR(Branchpoint);
SPUG_RCPTR(BuilderContextData);
SPUG_RCPTR(CleanupFrame);
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
SPUG_RCPTR(ModuleDef);
SPUG_RCPTR(Namespace);
SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDef);

SPUG_RCPTR(Context);

/**
 * Holds everything relevant to the current parse context.
 */
class Context : public spug::RCBase {
    private:
        typedef std::map<std::string, StrConstPtr> StrConstTable;
        
        // break and continue branchpoints
        BranchpointPtr breakBranch, continueBranch;

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

        struct GlobalData {
            StrConstTable strConstTable;
            TypeDefPtr classType,
                       voidType,
                       voidPtrType,
                       boolType,
                       byteptrType,
                       byteType,
                       int32Type,
                       int64Type,
                       uint32Type,
                       uint64Type,
                       intType,
                       uintType,
                       float32Type,
                       float64Type,
                       floatType,
                       vtableBaseType,
                       objectType,
                       stringType,
                       staticStringType,
                       overloadType;
            
            // just make sure the bootstrapped types are null
            GlobalData();
        } *globalData;
    
        Context(builder::Builder &builder, Scope scope,
                Context *parentContext,
                Namespace *ns
                );
        
        Context(builder::Builder &builder, Scope scope,
                GlobalData *globalData,
                Namespace *ns
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
         * Emit a variable definition in the context.
         */
        void emitVarDef(TypeDef *type, const parser::Token &name, 
                        Expr *initializer
                        );

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
        
        void dump(std::ostream &out, const std::string &prefix) const;
        void dump();
};

inline std::ostream &operator <<(std::ostream &out, const Context &context) {
    context.dump(out, "");
    return out;
}

}; // namespace model

#endif


