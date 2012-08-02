// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_CleanupFrame_h_
#define _model_CleanupFrame_h_

#include <spug/RCPtr.h>
#include <spug/RCBase.h>

namespace model {

SPUG_RCPTR(CleanupFrame);

class Context;
class Expr;
class FuncCall;
class VarDef;

/**
 * CleanupFrame is a collection of cleanup activities to perform at the end of 
 * a context - examples include the cleanup of variables at the end of a block 
 * or the cleanup of temporaries after evalutation of a statement.
 *
 * Cleanups are to be executed in the reverse order that they are added.
 */
class CleanupFrame : public spug::RCBase {
    public:
        CleanupFramePtr parent;
        Context *context;

        CleanupFrame(Context *context) : context(context) {}

        /**
         * Add a cleanup operation to the frame.
         */
        virtual void addCleanup(Expr *cleanupFuncCall) = 0;
        
        /**
         * Adds a cleanup for the given variable definition if one is needed.
         * @param aggregate if defined, this is the aggregate that varDef is a 
         *  member of.
         */
        void addCleanup(VarDef *varDef, Expr *aggregate = 0);
        
        /** 
         * Close the cleanup frame.  If the code is not terminal at this 
         * point, the cleanups will be emitted and executed.
         */
        virtual void close() = 0;
};

} // namespace model

#endif

