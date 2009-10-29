
#ifndef _model_FuncDef_h_
#define _model_FuncDef_h_

#include "VarDef.h"

namespace model {

SPUG_RCPTR(ArgDef);

SPUG_RCPTR(FuncDef);

class FuncDef : public VarDef {
    public:
        std::vector<ArgDefPtr> args;

        FuncDef(const std::string &name, size_t argCount) :
            // XXX need function types, but they'll probably be assigned after 
            // the fact.
            VarDef(0, name),
            args(argCount) {
        }
};

} // namespace model

#endif
