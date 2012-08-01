// Copyright 2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_ext_util_h_
#define _crack_ext_util_h_

namespace crack { namespace ext {

/**
 * Returns the crack proxy object for an object that we know to be wrapped in
 * a proxy.
 */
template <typename T>
inline T *getCrackProxy(void *wrapped) {
    return reinterpret_cast<T *>(
        reinterpret_cast<void **>(wrapped) - 1
    );
}

}} // namespace crack::ext

#endif
