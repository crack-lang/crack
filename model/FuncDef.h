
#ifndef _model_FuncDef_h_
#define _model_FuncDef_h_

#include "Def.h"

namespace builder {
    class Func;
}

namespace model {

SPUG_RCPTR(ArgDef);

SPUG_RCPTR(FuncDef);

// XXX why isn't it just VarDef?
class FuncDef : public Def {
    public:
        std::vector<ArgDefPtr> args;

        FuncDef(const char *name, size_t argCount) :
            Def(name),
            args(argCount) {
        }
};

} // namespace model

#endif
