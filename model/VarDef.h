
#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(VarDef);

// XXX I don't think there's any kind of definition that is not a "VarDef" - 
// we should probably collapse this logic into Def.
class VarDef : public spug::RCBase {
    public:
        Context *context;
        TypeDefPtr type;
        std::string name;

        VarDef(const TypeDefPtr &type, const std::string &name);
};

} // namespace model

#endif
