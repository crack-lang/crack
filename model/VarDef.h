
#ifndef _model_VarDef_h_
#define _model_VarDef_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(VarDefImpl);
class Context;
SPUG_RCPTR(TypeDef);

SPUG_RCPTR(VarDef);

// Variable definition.  All names in a context (including functions and 
// types) are derived from VarDef's.
class VarDef : public spug::RCBase {
    public:
        Context *context;
        TypeDefPtr type;
        std::string name;
        VarDefImplPtr impl;

        VarDef(const TypeDefPtr &type, const std::string &name);
};

} // namespace model

#endif
