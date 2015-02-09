// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_BModuleDef_h_
#define _builder_llvm_BModuleDef_h_

#include "util/SourceDigest.h"
#include "model/ModuleDef.h"
#include "model/ImportedDef.h"
#include <spug/RCPtr.h>
#include <string>
#include <vector>
#include <map>

namespace model {
    class Context;
}

namespace llvm {
    class Module;
}

namespace builder {
namespace mvll {

SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(BStrConst);

class BModuleDef : public model::ModuleDef {

public:
    // Definitions created in the module that are not part of its defs.
    std::vector<model::VarDefPtr> orphanedDefs;

    // primitive cleanup function
    void (*cleanup)();
    llvm::Module *rep;
    int repId;
    std::vector<BStrConstPtr> stringConstants;

    // list of modules imported by this one, along with its imported symbols
    typedef std::map<BModuleDef*, model::ImportedDefVec > ImportListType;
    ImportListType importList;
    // list of shared libraries imported, along with imported symbols
    typedef std::map<std::string, model::ImportedDefVec > ShlibImportListType;
    ShlibImportListType shlibImportList;

    BModuleDef(const std::string &canonicalName,
               model::Namespace *parent,
               llvm::Module *rep0,
               int repId
               );

    ~BModuleDef();

    void callDestructor() {
        if (cleanup)
            cleanup();
    }

    void recordDependency(ModuleDef *other);

    /**
     * Clears the 'rep' and 'module' fields from all constants so there's no
     * chance that they can coincidentally match another module.
     */
    void clearRepFromConstants();

    virtual void runMain(Builder &builder);
};

} // end namespace builder::vmll
} // end namespace builder

#endif
