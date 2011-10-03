// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_llvm_SourceDigest_h_
#define _builder_llvm_SourceDigest_h_

#include <string>
#include "md5.h"

namespace builder {

class SourceDigest {

    // MD5
    typedef md5_byte_t digest_byte_t;
    static const int digest_size = 16;

    SourceDigest::digest_byte_t digest[SourceDigest::digest_size];

public:

    SourceDigest(void);

    static SourceDigest fromFile(const std::string &path);
    static SourceDigest fromHex(const std::string &d);

    std::string asHex() const;

    bool operator==(const SourceDigest &other) const;
    bool operator!=(const SourceDigest &other) const;

};

} // end namespace builder

#endif
