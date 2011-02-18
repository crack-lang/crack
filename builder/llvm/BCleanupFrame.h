// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BCleanupFrame_h_
#define _builder_llvm_BCleanupFrame_h_

#include "model/CleanupFrame.h"
#include "model/Expr.h"
#include "model/Context.h"
#include <spug/RCPtr.h>
#include <llvm/Support/IRBuilder.h>
#include <list>

namespace builder {
namespace mvll {

SPUG_RCPTR(BCleanupFrame)

class BCleanupFrame : public model::CleanupFrame {
private:
    struct Cleanup {
        model::ExprPtr action;
        llvm::BasicBlock *unwindBlock;
        
        Cleanup(model::ExprPtr action) : 
            action(action),
            unwindBlock(0) {
        }
    };
        
public:
    typedef std::list<Cleanup> CleanupList;
    CleanupList cleanups;

    BCleanupFrame(model::Context *context) :
        CleanupFrame(context) {
    }

    virtual void addCleanup(model::Expr *cleanup) {
        cleanups.push_front(Cleanup(cleanup));
    }

    virtual void close() {
        context->emittingCleanups = true;
        for (CleanupList::iterator iter = cleanups.begin();
             iter != cleanups.end();
             ++iter
             )
            iter->action->emit(*context);
        context->emittingCleanups = false;
    }
    
    llvm::BasicBlock *emitUnwindCleanups(llvm::BasicBlock *next);
    
    void clearCachedCleanups();
};

} // end namespace builder::vmll
} // end namespace builder

#endif
