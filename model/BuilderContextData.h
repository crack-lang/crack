

#ifndef _model_BuilderContextData_h_
#define _model_BuilderContextData_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(BuilderContextData);

/**
 * Base class for Builder-specific information to be stored in a Context 
 * object.
 */
class BuilderContextData : public spug::RCBase {
    public:
        BuilderContextData() {}
};

} // namespace model

#endif
