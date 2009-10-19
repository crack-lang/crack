
#ifndef _model_BuilderVarDefData_h_
#define _model_BuilderVarDefData_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(BuilderVarDefData);

/**
 * Base class for Builder-specific information to be stored in a VarDef
 * object.
 */
class BuilderVarDefData : public spug::RCBase {
    public:
        BuilderVarDefData() {}
};

} // namespace model

#endif
