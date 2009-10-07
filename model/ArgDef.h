
#ifndef _model_ArgDef_h_
#define _model_ArgDef_h_

#include "Def.h"

namespace model {

SPUG_RCPTR(TypeDef);

SPUG_RCPTR(ArgDef);

class ArgDef : public Def {
    public:
        TypeDefPtr type;
        ArgDef(const char *name, const TypeDefPtr &type) :
            Def(name), 
            type(type) {
        }
};

} // namespace model

#endif
