// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "SourceDigest.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "Hasher.h"

using namespace crack::util;
using namespace std;

namespace {

    void md5_hashSourceText(istream &src, SourceDigest &digest) {
        Hasher hasher;

        #define MD5_SOURCE_PAGE_SIZE 1024
        char buf[MD5_SOURCE_PAGE_SIZE];

        // XXX do we want to skip whitespace and comments?
        while (!src.eof() && src.good()) {
            src.read(buf, MD5_SOURCE_PAGE_SIZE);
            hasher.add(buf, src.gcount());
        }

        digest = hasher.getDigest();
    }

}

SourceDigest::SourceDigest() {
    memset(&digest, 0, SourceDigest::digest_size);
}

SourceDigest SourceDigest::fromFile(const std::string &path) {

    ifstream src;
    src.open(path.c_str());
    if (!src.good())
        return SourceDigest();

    // MD5
    SourceDigest d;
    md5_hashSourceText(src, d);
    return d;

}

SourceDigest SourceDigest::fromStr(const string &str) {
    SourceDigest d;
    istringstream src(str);
    md5_hashSourceText(src, d);
    return d;
}

SourceDigest SourceDigest::fromHex(const std::string &d) {
    SourceDigest result;
    if (d.length() != SourceDigest::digest_size*2)
        return result;
    const char *buf = d.c_str();
    char pos[3];
    pos[2] = 0;
    for (int di = 0; di < SourceDigest::digest_size; ++di) {
        pos[0] = *(buf+di*2);
        pos[1] = *((buf+di*2)+1);
        result.digest[di] = (digest_byte_t)strtol((const char*)&pos, NULL, 16);
    }
    return result;
}

string SourceDigest::asHex() const {

    char buf[SourceDigest::digest_size*2];
    for (int di = 0; di < SourceDigest::digest_size; ++di)
        sprintf(buf + di * 2, "%02x", digest[di]);

    return string(buf, SourceDigest::digest_size*2);

}

bool SourceDigest::operator==(const SourceDigest &other) const {
    return (memcmp(digest, other.digest, SourceDigest::digest_size) == 0);
}

bool SourceDigest::operator!=(const SourceDigest &other) const {
    return (memcmp(digest, other.digest, SourceDigest::digest_size) != 0);
}

