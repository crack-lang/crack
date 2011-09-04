// Copyright 2009 Google Inc.

#ifndef _model_VarDefImpl_h_
#define _model_VarDefImpl_h_

#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class AssignExpr;
SPUG_RCPTR(ResultExpr);
class VarRef;

SPUG_RCPTR(VarDefImpl);

/**
 * Variable definition implementation that knows how to emit a reference to 
 * the variable.
 */
class VarDefImpl : public spug::RCBase {
    public:
        VarDefImpl() {}
        
        virtual ResultExprPtr emitRef(Context &context, VarRef *var) = 0;
        
        virtual ResultExprPtr emitAssignment(Context &context,
                                             AssignExpr *assign
                                             ) = 0;

        virtual bool hasInstSlot() const = 0;
};

} // namespace model

#endif
