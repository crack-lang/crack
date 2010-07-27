// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_TEMPLATE_h_
#define _builder_llvm_TEMPLATE_h_

#include "model/CleanupFrame.h"
#include "model/Expr.h"
#include "model/Context.h"
#include <spug/RCPtr.h>
#include <vector>

namespace builder {
namespace mvll {

SPUG_RCPTR(BCleanupFrame)

class BCleanupFrame : public model::CleanupFrame {
public:
    std::vector<model::ExprPtr> cleanups;

    BCleanupFrame(model::Context *context) : CleanupFrame(context) {}

    virtual void addCleanup(model::Expr *cleanup) {
        cleanups.insert(cleanups.begin(), cleanup);
    }

    virtual void close() {
        context->emittingCleanups = true;
        for (std::vector<model::ExprPtr>::iterator iter = cleanups.begin();
        iter != cleanups.end();
        ++iter)
            (*iter)->emit(*context);
        context->emittingCleanups = false;
    }
};

} // end namespace builder::vmll
} // end namespace builder

#endif
