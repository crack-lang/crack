// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ProtoBuf_h_
#define _model_ProtoBuf_h_

namespace model {

class ProtoBuf {
    public:
        enum { varInt = 0, bits64 = 1, string = 2, ref = 3, bits32 = 5 };
};

} // namespace model

// These macros try to take away the boilerplate aspects of dealing with
// nested protobuf files.
// Use them like this:
//    CRACK_PB_BEGIN(deserializer, 256, optional)
//      CRACK_PB_FIELD(1, varInt)
//          foo = optionalDeser.readUInt("foo");
//          break;
//      CRACK_PB_FIELD(2, string)
//          bar = optionalDeser.readString(16, "bar");
//          break;
//    CRACK_PB_END
//
// Note that:
//  - the break statements are very important.
//  - CRACK_PB_BEGIN() defines a set of variables whose names begin with its
//    third argument (which is also the name of the field that it will read).
//    The most important of them is "*Deser" (optionalDeser in the example).
//    This is a nested deserializer.

#define CRACK_PB_KEY(id, type) ((id << 3) | (model::ProtoBuf::type))
#define CRACK_PB_BEGIN(deser, maxSize, sectionName) \
    { \
    model::NestedDeserializer sectionName##Deser(deser, maxSize, #sectionName); \
    bool sectionName##EOF; \
    int sectionName##Header; \
    while ((sectionName##Header = \
                sectionName##Deser.readUInt(#sectionName ".header", \
                                            &sectionName##EOF)) || \
           !sectionName##EOF) { \
        switch (sectionName##Header) {

#define CRACK_PB_FIELD(id, type) case CRACK_PB_KEY(id, type):
#define CRACK_PB_END }}}

#endif
