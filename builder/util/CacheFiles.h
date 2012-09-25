// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_CacheFiles_h_
#define _builder_llvm_CacheFiles_h_

#include "model/Context.h"
#include <string>

namespace builder {

class BuilderOptions;

std::string getCacheFilePath(BuilderOptions* o,
                             model::Construct &construct,
                             const std::string &path,
                             const std::string &destExt
                             );

bool initCacheDirectory(BuilderOptions *o, model::Construct &construct);

} // end namespace builder

#endif
