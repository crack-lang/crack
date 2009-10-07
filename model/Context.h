
#ifndef _model_Context_h_
#define _model_Context_h_

#include <map>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace builder {
    class Builder;
}

namespace model {

SPUG_RCPTR(Def);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(Context);

/**
 * Holds everything relevant to the current parse context.
 */
class Context : public spug::RCBase {
    
    private:
        typedef std::map<std::string, DefPtr> DefMap;
        DefMap defs;

        typedef std::map<std::string, StrConstPtr> StrConstTable;
    public:
        
        ContextPtr parent;
        builder::Builder &builder;

        struct GlobalData {
            StrConstTable strConstTable;
            TypeDefPtr byteptrType,
                       intType,
                       int32Type;
        } *globalData;
    
        Context(builder::Builder &builder);

        DefPtr lookUp(const std::string &varName);
        void createModule(const char *name);
        void addDef(const DefPtr &def);
        
        /** Get or create a string constsnt. */
        StrConstPtr getStrConst(const std::string &value);
        
};

}; // namespace model

#endif


