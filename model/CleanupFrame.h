// Copyright 2009 Google Inc.

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
         */
        void addCleanup(VarDef *varDef);
        
        /** 
         * Close the cleanup frame.  If the code is not terminal at this 
         * point, the cleanups will be emitted and executed.
         */
        virtual void close() = 0;
};

} // namespace model

#endif

