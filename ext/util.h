// Copyright 2011 Google Inc.

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
