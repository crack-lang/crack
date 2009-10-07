
#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include <string>

namespace model {

class VarDef : public Def {
    public:
        std::string name;
        VarDef(const char *name) : name(name) {}
};

} // namespace model

#endif
