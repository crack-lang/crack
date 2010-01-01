
#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace builder {
    class Builder;
}

namespace model {

SPUG_RCPTR(BuilderContextData);
SPUG_RCPTR(CleanupFrame);
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
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
                       intType,
                       int32Type;
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
         * Returns the Overload Definition for the given name for the current 
         * context.  This will return null if:
         *   1) there are no functions named varName
         *   2) the local context contains a non-func variable named varName.
         */
        OverloadDefPtr aggregateOverloads(const std::string &varName);

        VarDefPtr lookUp(const std::string &varName, bool recurse = true);
        
        /**
         * Looks up a function matching the given expression list.
         * 
         * @param varName the function name
         * @param vals list of parameter expressions.  These will be converted 
         *  to conversion expressions of the correct type for a match.
         */
        FuncDefPtr lookUp(const std::string &varName,
                          std::vector<ExprPtr> &vals
                          );
        void createModule(const char *name);
        void addDef(VarDef *def);
        
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
        
        /** Get or create a string constsnt. */
        StrConstPtr getStrConst(const std::string &value);

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
};

}; // namespace model

#endif


