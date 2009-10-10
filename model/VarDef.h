
#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include "Def.h"

namespace model {

SPUG_RCPTR(TypeDef);

SPUG_RCPTR(VarDef);

// XXX I don't think there's any kind of definition that is not a "VarDef" - 
// we should probably collapse this logic into Def.
class VarDef : public Def {
    public:
        TypeDefPtr type;
        VarDef(const TypeDefPtr &type, const std::string &name);
};

} // namespace model

#endif
