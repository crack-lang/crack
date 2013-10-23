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

#include "spug/RCBase.h"
#include "spug/RCPtr.h"
#include "util/Hasher.h"

namespace spug {
    SPUG_RCPTR(RCBase);
}

namespace model {

class Context;

class Deserializer {
    private:
        std::istream &src;

        // the deserializer's object map
        struct ObjMap : public std::map<int, spug::RCBasePtr>,
                        public spug::RCBase {
            ObjMap() {}
        };
        SPUG_RCPTR(ObjMap);
        ObjMapPtr objMap;

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

        // Allows an object reader to pass back information to the
        // higher-level calling code for use after the object is deserialized.
        // This is copied into the ReadObjectResult structure returned by
        // readObject().  This doesn't get communicated across child
        // deserializers.
        int userData;

        // Hash of all of the data read while digestEnabled is true.
        crack::util::Hasher hasher;

        // If true, new data read is added to the digest.
        bool digestEnabled;

        Deserializer(std::istream &src) :
            src(src),
            objMap(new ObjMap()),
            context(0),
            digestEnabled(false) {
        }

        Deserializer(std::istream &src, Context *context) :
            src(src),
            objMap(new ObjMap()),
            context(context),
            digestEnabled(false) {
        }

        Deserializer(Deserializer &parent, std::istream &src) :
            src(src),
            objMap(parent.objMap),
            context(parent.context),
            digestEnabled(false) {
        }

        /**
         * Read a protobuf-style Varint from the stream. If the end-of-stream
         * is encountered and 'eof' is not null, returns 0 and *eof will be
         * set to true.  If it is null and an end-of-stream is encountered,
         * throws a DeserializationError.
         */
        unsigned int readUInt(const char *name, bool *eof = 0);

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

        struct ReadObjectResult {
            spug::RCBasePtr object;

            // True if we just read the definition of the object (if this was
            // the first time it was encountered)
            bool definition;

            // Field to allow first-stage deserializers to pass information
            // back to a second stage.  Initialized to zero.
            int userData;

            ReadObjectResult(spug::RCBasePtr object, bool definition,
                             int userData) :
                object(object),
                definition(definition),
                userData(userData) {
            }
        };

        /**
         * Read the next object from the stream.  This returns a pointer to an
         * existing object if the object possibly calling reader.read() to
         * deserialize the object from the stream.
         */
        ReadObjectResult readObject(const ObjectReader &reader,
                                    const char *name
                                    );

        /**
         * Register an object under the specified id without having read it.
         */
        void registerObject(int id, spug::RCBase *object);

        /**
         * Returns a registered object, null if the object does not eist.
         */
        spug::RCBasePtr getObject(int id) const;

        /**
         * Read an ISO-8859 double from the stream.  These are presumed to be
         * a fixed width of 8 bytes.
         */
        double readDouble(const char *name);
};

}

#endif
