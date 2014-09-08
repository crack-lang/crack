// Copyright 2009-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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

        /**
         * Emit the address of the variable for variables where this is 
         * possible.
         */        
        virtual void emitAddr(Context &context, VarRef *var) = 0;

        virtual bool hasInstSlot() const = 0;
        
        virtual int getInstSlot() const = 0;

        /**
         * True if the variable is an instance variable (and thus needs a 
         * receiver).
         * Unlike hasInstSlot(), this is also true for offset-based instance 
         * variables in extensions.
         */
        virtual bool isInstVar() const = 0;
};

} // namespace model

#endif
