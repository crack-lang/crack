// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>
#include <vector>
#include <map>

#include <llvm/Support/MemoryBuffer.h>

namespace llvm {
    class Module;
    class MDNode;
    class Value;
    class Function;
}

namespace model {
    class Namespace;
    class TypeDef;
    class VarDef;
}

namespace builder {

class BuilderOptions;

namespace mvll {

SPUG_RCPTR(BModuleDef);
SPUG_RCPTR(BTypeDef);
SPUG_RCPTR(LLVMBuilder);

class Cacher {

    BModuleDefPtr modDef;
    model::ContextPtr context;
    model::Context &parentContext;
    builder::BuilderOptions *options;
    builder::mvll::LLVMBuilderPtr builder;

    // vardefs which were created as a result of shared lib import
    // we skip these in crack_defs
    std::map<std::string, bool> shlibImported;

protected:
    bool readImports();

public:

    Cacher(model::Context &c, builder::BuilderOptions *o,
           BModuleDef *m = NULL
           );

    void writeMetadata();
    bool getCacheFile(const std::string &canonicalName,
                      llvm::OwningPtr<llvm::MemoryBuffer> &fileBuf
                      );
    BModuleDefPtr maybeLoadFromCache(
        const std::string &canonicalName,
        llvm::OwningPtr<llvm::MemoryBuffer> &fileBuf
    );
};

} // end namespace builder::vmll
} // end namespace builder

#endif
