// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BCleanupFrame_h_
#define _builder_llvm_BCleanupFrame_h_

#include "model/CleanupFrame.h"
#include "model/Expr.h"
#include "model/Context.h"
#include <spug/RCPtr.h>
#include <llvm/Support/IRBuilder.h>
#include <list>
#include "BBuilderContextData.h"

namespace builder {
namespace mvll {

SPUG_RCPTR(BCleanupFrame)

class BCleanupFrame : public model::CleanupFrame {
private:
    struct Cleanup {
        bool emittingCleanups;
        model::ExprPtr action;
        llvm::BasicBlock *unwindBlock, *landingPad;
        
        Cleanup(model::ExprPtr action) :
            emittingCleanups(false),
            action(action),
            unwindBlock(0),
            landingPad(0) {
        }
    };

    llvm::BasicBlock *landingPad;

public:
    typedef std::list<Cleanup> CleanupList;
    CleanupList cleanups;

    BCleanupFrame(model::Context *context) :
        CleanupFrame(context),
        landingPad(0) {
    }

    virtual void addCleanup(model::Expr *cleanup) {
        cleanups.push_front(Cleanup(cleanup));
    }

    virtual void close();
    
    llvm::BasicBlock *emitUnwindCleanups(llvm::BasicBlock *next);
    
    /** 
     * Returns a cached landing pad for the cleanup.   LLVM requires a call to 
     * the selector to be in the unwind block for an invoke, so we have to 
     * keep one of these for every cleanup block that needs one.
     */
    llvm::BasicBlock *getLandingPad(llvm::BasicBlock *block, 
                                    BBuilderContextData::CatchData *cdata
                                    );
    
    void clearCachedCleanups();
};

} // end namespace builder::vmll
} // end namespace builder

#endif
