
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
    
    private:
        typedef std::map<std::string, VarDefPtr> VarDefMap;
        VarDefMap defs;

        typedef std::map<std::string, StrConstPtr> StrConstTable;
    public:

        // context scope - this is used to control how variables defined in 
        // the scope are stored.
        enum Scope {
            module,
            instance,
            local
        };
        
        ContextPtr parent;
        builder::Builder &builder;
        BuilderContextDataPtr builderData;
        Scope scope;
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
                Context *parentContext = 0
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
        
        /** Get or create a string constsnt. */
        StrConstPtr getStrConst(const std::string &value);
        
};

}; // namespace model

#endif


