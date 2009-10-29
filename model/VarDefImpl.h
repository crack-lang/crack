
#ifndef _model_VarDefImpl_h_
#define _model_VarDefImpl_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

SPUG_RCPTR(Expr);
SPUG_RCPTR(VarDefImpl);

/**
 * Variable definition implementation that knows how to emit a reference to 
 * the variable.
 */
class VarDefImpl : public spug::RCBase {
    public:
        VarDefImpl() {}
        
        virtual void emitRef(Context &context) = 0;
        
        virtual void emitAssignment(Context &context, 
                                    const ExprPtr &expr
                                    ) = 0;
};

} // namespace model

#endif
