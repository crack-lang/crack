// Copyright 2011 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_BModuleDef_h_
#define _builder_llvm_BModuleDef_h_

#include "builder/util/SourceDigest.h"
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

class BModuleDef : public model::ModuleDef {

public:
    // primitive cleanup function
    void (*cleanup)();
    llvm::Module *rep;

    // source text hash code, used for caching
    SourceDigest digest;

    // list of modules imported by this one, along with its imported symbols
    typedef std::map<BModuleDef*, model::ImportedDefVec > ImportListType;
    ImportListType importList;
    // list of shared libraries imported, along with imported symbols
    typedef std::map<std::string, model::ImportedDefVec > ShlibImportListType;
    ShlibImportListType shlibImportList;

    BModuleDef(const std::string &canonicalName,
               model::Namespace *parent,
               llvm::Module *rep0
               ) :
            ModuleDef(canonicalName, parent),
            cleanup(0),
            rep(rep0),
            digest(),
            importList()
    {
    }

    void callDestructor() {
        if (cleanup)
            cleanup();
    }

    void recordDependency(ModuleDef *other) {
        // reference the entry for this module to create an empty import list
        // if none currently exists.
        importList[BModuleDefPtr::acast(other)];
    }

    virtual bool matchesSource(const std::string &source) {
        return digest == SourceDigest::fromFile(source);
    }
};

} // end namespace builder::vmll
} // end namespace builder

#endif
