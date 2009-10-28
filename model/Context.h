
#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace builder {
    class Builder;
}

namespace model {

SPUG_RCPTR(BuilderContextData);
SPUG_RCPTR(VarDef);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(TypeDef);

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

        VarDefPtr lookUp(const std::string &varName);
        void createModule(const char *name);
        void addDef(const VarDefPtr &def);
        
        /** Get or create a string constsnt. */
        StrConstPtr getStrConst(const std::string &value);
        
};

}; // namespace model

#endif


