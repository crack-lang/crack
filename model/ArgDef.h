
#ifndef _model_ArgDef_h_
#define _model_ArgDef_h_

#include "VarDef.h"

namespace model {

SPUG_RCPTR(TypeDef);

SPUG_RCPTR(ArgDef);

class ArgDef : public VarDef {
    public:
        ArgDef(TypeDef *type, const std::string &name) :
            VarDef(type, name) {
        }
};

} // namespace model

#endif
