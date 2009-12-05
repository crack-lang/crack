
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
        
        // indicates the depth to recurse to when generating cleanup code.
        enum Depth {
            block, // do not recurse - just this context.
            function // recurse all the way to the function context
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
        
        // this is the return type for a function context, and the class type 
        // for a class context.  XXX there is a reference cycle between the 
        // class and its context.
        TypeDefPtr returnType;

        struct GlobalData {
            StrConstTable strConstTable;
            TypeDefPtr voidType,
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

        VarDefPtr lookUp(const std::string &varName);
        
        /** Looks up a function matching the given expression list. */
        FuncDefPtr lookUp(const std::string &varName,
                          const std::vector<ExprPtr> &vals
                          );
        void createModule(const char *name);
        void addDef(const VarDefPtr &def);
        
        /** Funcs to iterate over the set of definitions. */
        /// @{
        VarDefMap::iterator beginDefs() { return defs.begin(); }
        VarDefMap::iterator endDefs() { return defs.end(); }
        /// @}
        
        /** Get or create a string constsnt. */
        StrConstPtr getStrConst(const std::string &value);
        
        /** 
         * Emit all of the cleanup code for given local scope.
         * @param depth how deep in the context stack to clean.
         */
        void emitCleanups(Depth depth);
        
};

}; // namespace model

#endif


