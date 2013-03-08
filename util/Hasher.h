// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// MD5 hasher.

#ifndef _crack_util_Hasher_h_
#define _crack_util_Hasher_h_

#include <stdint.h>
#include <unistd.h>  // for size_t
#include "md5.h"

namespace crack { namespace util {

class SourceDigest;

/**
 * Hasher lets you compute an MD5 digest from an arbitrary data stream.  It
 * provides two "add()" methods to allow you to add a single byte or a block
 * of memory to the digest.  The digest can be obtained using getDigest().
 * The behavior of this class when add() is called after getDigest() is
 * currently undefined.
 */
class Hasher {
    private:
        md5_state_t state;

    public:
        Hasher();

        /**
         * Add a new single byte to the information in the digest.
         */
        void add(uint8_t byte);

        /**
         * Add a region of memory to the information in the digest.
         */
        void add(const void *data, size_t size);

        /**
         * Returns the digest for the information read.
         */
        SourceDigest getDigest();
};

}} // namespace crack::util

#endif
