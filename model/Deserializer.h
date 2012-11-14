// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Deserializer_h_
#define _model_Deserializer_h_

#include <string>
#include <map>

#include "spug/RCPtr.h"

namespace spug {
    SPUG_RCPTR(RCBase);
}

namespace model {

class Context;

class Deserializer {
    private:
        std::istream &src;

        // the deserializer's object map
        typedef std::map<int, spug::RCBasePtr> ObjMap;
        ObjMap objMap;

    public:

        /**
         * interface for object reader classes - these know how to read a
         * specific object from the stream and are used as a callback for
         * readObject().
         */
        struct ObjectReader {
            virtual spug::RCBasePtr read(Deserializer &src) const = 0;
        };

        Context *context;

        Deserializer(std::istream &src) : src(src) {}
        Deserializer(std::istream &src, Context *context) :
            src(src),
            context(context) {
        }

        unsigned int readUInt(const char *name);

        /**
         * Read a sized blob (block of binary data) from the stream.
         * If a buffer is provided, it attempts to use the buffer.  Otherwise
         * it returns a new, allocated buffer containing the data
         * This function always returns a pointer to the buffer, the caller
         * should delete[] the buffer if it is not the input
         * buffer.
         * @param size (input/output) on input this is this size of 'buffer'.
         *          On output it is the size of the blob that was read.  The
         *          input value is ignore if 'buffer' is null.
         * @param buffer If specified, this is a pointer to a buffer for the
         *          blob to be stored in.  If this is not null, 'size' must be
         *          provided.
         */
        char *readBlob(size_t &size, char *buffer, const char *name);

        /**
         * Read a block of binary data as a string.  expectedMaxSize is a
         * reasonable size for the buffer - if it is greater then more space
         * will be allocated.
         * This implements the common case of creating a string from the blob.
         */
        std::string readString(size_t expectedMaxSize, const char *name);

        /**
         * Read the next object from the stream.  This returns a pointer to an
         * existing object if the object possibly calling reader.read() to
         * deserialize the object from the stream.
         */
        spug::RCBasePtr readObject(const ObjectReader &reader,
                                   const char *name
                                   );
};

}

#endif
