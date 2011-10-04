// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_Cacher_h_
#define _builder_llvm_Cacher_h_

#include "model/Context.h"
#include <string>

namespace llvm {
    class Module;
}

namespace builder {

class BuilderOptions;

namespace mvll {

class BModuleDef;

class Cacher {

    BModuleDef *modDef;
    model::Context &context;
    const builder::BuilderOptions *options;

protected:
    void writeBitcode(const std::string &path);
    void writeMetadata();
    bool readMetadata();

public:

    Cacher(model::Context &c, builder::BuilderOptions* o, BModuleDef *m = NULL):
        modDef(m), context(c), options(o) { }

    BModuleDef *maybeLoadFromCache(const std::string &canonicalName,
                                   const std::string &path);
    void saveToCache();

};

} // end namespace builder::vmll
} // end namespace builder

#endif
