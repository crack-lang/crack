// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Hasher.h"

#include "SourceDigest.h"

using namespace crack::util;

Hasher::Hasher() {
    md5_init(&state);
}

void Hasher::add(uint8_t byte) {
    md5_append(&state, reinterpret_cast<const md5_byte_t *>(&byte), 1);
}

void Hasher::add(const void *data, size_t size) {
    md5_append(&state, reinterpret_cast<const md5_byte_t *>(data), size);
}

SourceDigest Hasher::getDigest() {
    SourceDigest result;
    md5_finish(&state, result.digest);
    return result;
}
