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
SPUG_RCPTR(OverloadDef);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(TypeDef);
SPUG_RCPTR(VarDef);

SPUG_RCPTR(Context);

/**
 * Holds everything relevant to the current parse context.
 */
class Context : public spug::RCBase {
    
    public:
        typedef std::map<std::string, VarDefPtr> VarDefMap;
    
    private:
        VarDefMap defs;
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
        
        typedef std::vector<ContextPtr> ContextVec;
        ContextVec parents;

        builder::Builder &builder;
        BuilderContextDataPtr builderData;
        Scope scope;
        
        // true if the context has been completely defined (so that we can 
        // determine whether to emit references or placeholders for instance 
        // variable references and assignments)
        bool complete;
        
        // true if the context is the outermost context of a function.
        bool toplevel;
        
        // true if we are currently in the process of emitting cleanups - 
        // prevents us from trying to add cleanups on the expressions that we 
        // are cleaning up.
        bool emittingCleanups;
        
        // if true, a terminal statement has been emitted in the context.
        bool terminal;
        
        // this is the return type for a function context, and the class type 
        // for a class context.  XXX there is a reference cycle between the 
        // class and its context.
        TypeDefPtr returnType;
        
        // the current cleanup frame.
        CleanupFramePtr cleanupFrame;

        struct GlobalData {
            StrConstTable strConstTable;
            TypeDefPtr voidType,
                       voidPtrType,
                       boolType,
                       byteptrType,
                       int32Type,
                       int64Type,
                       uint32Type,
                       uint64Type,
                       intType,
                       uintType,
                       vtableBaseType,
                       objectType,
                       stringType,
                       staticStringType,
                       overloadType;
            
            // just make sure the bootstrapped types are null
            GlobalData();
        } *globalData;
    
        Context(builder::Builder &builder, Scope scope,
                Context *parentContext
                );
        
        Context(builder::Builder &builder, Scope scope,
                GlobalData *globalData
                );
        
        ~Context();
        
        /**
         * Create a new subcontext with a different scope from the parent 
         * context.
         */
        ContextPtr createSubContext(Scope newScope);

        /**
         * Create a new subcontext in the same scope.
         */
        ContextPtr createSubContext() {
            return createSubContext(scope);
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
         * Returns the parent of the context.  This function can only be used 
         * on contexts with a exactly one parent.
         */
        ContextPtr getParent();
        
        /**
         * Returns true if the context encloses the "other" context - a 
         * context encloses another context if it is an ancestor of the other 
         * context.
         */
        bool encloses(const Context &other) const;

        /**
         * Returns the Overload Definition for the given name for the current 
         * context.  Creates an overload definition if one does not exist.
         * 
         * @param varName the overload name.
         */
        OverloadDefPtr getOverload(const std::string &varName);

        VarDefPtr lookUp(const std::string &varName, bool recurse = true);
        
        /**
         * Looks up a function matching the given expression list.
         * 
         * @param context the current context (distinct from the lookup 
         *  context)
         * @param varName the function name
         * @param vals list of parameter expressions.  These will be converted 
         *  to conversion expressions of the correct type for a match.
         */
        FuncDefPtr lookUp(Context &context,
                          const std::string &varName,
                          std::vector<ExprPtr> &vals
                          );
        
        /**
         * Look up a function with no arguments.  This is provided as a 
         * convenience, as in this case we don't need to pass the call context.
         */
        FuncDefPtr lookUpNoArgs(const std::string &varName);
        
        ModuleDefPtr createModule(const std::string &name);
        void addDef(VarDef *def);
        
        /** 
         * Remove a definition.  Intended for use with stubs - "def" must not 
         * be an OverloadDef. 
         */
        void removeDef(VarDef *def);
        
        /**
         * Adds a definition to the context, but does not make the definition's 
         * context the context.  This is used for importing symbols into a 
         * module context.
         */
        void addAlias(VarDef *def);
        void addAlias(const std::string &name, VarDef *def);
        
        /**
         * Replace an existing defintion with the new definition.
         * This is only used to replace a StubDef with an external function 
         * definition.
         */
        void replaceDef(VarDef *def);
        
        /** Funcs to iterate over the set of definitions. */
        /// @{
        VarDefMap::iterator beginDefs() { return defs.begin(); }
        VarDefMap::iterator endDefs() { return defs.end(); }
        /// @}
        
        /** 
         * Get or create a string constant.  This can be either a
         * "StaticString(StrConst, uint size)" expression if StaticString is 
         * defined, or a simple StrConst if it is not.
         */
        ExprPtr getStrConst(const std::string &value);

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
};

inline std::ostream operator <<(std::ostream &out, const Context &context) {
    context.dump(out, "");
}

}; // namespace model

#endif


