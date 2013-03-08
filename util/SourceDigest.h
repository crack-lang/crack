// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_SourceDigest_h_
#define _builder_llvm_SourceDigest_h_

#include <stdint.h>
#include <string>
#include "md5.h"

namespace crack { namespace util {

class Hasher;

/**
 * Class for creating MD5 digests.
 */
class SourceDigest {

    friend class Hasher;

    // MD5
    typedef md5_byte_t digest_byte_t;
    static const int digest_size = 16;

    SourceDigest::digest_byte_t digest[SourceDigest::digest_size];

public:

    SourceDigest(void);

    static SourceDigest fromFile(const std::string &path);
    static SourceDigest fromHex(const std::string &d);

    /**
     * Create a source digest of a string.
     */
    static SourceDigest fromStr(const std::string &d);

    std::string asHex() const;

    bool operator==(const SourceDigest &other) const;
    bool operator!=(const SourceDigest &other) const;

};

}} // end namespace crack::util

#endif
