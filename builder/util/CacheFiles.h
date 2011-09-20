// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_CacheFiles_h_
#define _builder_llvm_CacheFiles_h_

#include "model/Context.h"
#include <string>

namespace builder {

class BuilderOptions;

std::string getCacheFilePath(model::Context &c,
                             const BuilderOptions* o,
                             const std::string &canonicalName,
                             const std::string &ext);

} // end namespace builder

#endif
