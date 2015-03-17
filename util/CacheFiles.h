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
}

namespace crack { namespace util {

std::string getCacheFilePath(builder::BuilderOptions* o,
                             model::Construct &construct,
                             const std::string &path,
                             const std::string &destExt
                             );

bool initCacheDirectory(builder::BuilderOptions *o,
                        model::Construct &construct
                        );

/**
 * Move file 'src' to 'dst' (does a remove of 'dst' followed by a link(src,
 * dst).  Returns true if successful, false if not.
 * This will attempt to delete both files if unable to create a link.
 */
bool move(const std::string &src, const std::string &dst);

}} // end namespace crack::util

#endif
