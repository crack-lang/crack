// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Serializer_h_
#define _model_Serializer_h_

#include <map>
#include <string>
#include "spug/RCBase.h"
#include "spug/RCPtr.h"
#include "util/Hasher.h"

namespace model {

class ModuleDef;

class Serializer {
    private:
        std::ostream &dst;

        // mapping from a pointer to the object id associated with that
        // pointer.  This is part of the mechanism that allows us to serialize
        // an object that is used in multiple locations only the first time it
        // is used.
        struct ObjMap : public std::map<const spug::RCBase *, int>,
                        public spug::RCBase {
            int lastId;
            ObjMap() : lastId(0) {}
        };
        SPUG_RCPTR(ObjMap);
        ObjMapPtr objMap;

    public:

        // Sets the current value of the 'digestEnabled' flag while it in
        // scope, restores it during destruction.
        // This is a template so we can use it on both Serializer and
        // Deserializer.
        template <typename T>
        struct StackFrame {
            StackFrame(T &ser, bool digestEnabled) :
                ser(ser),
                oldVal(ser.digestEnabled) {

                ser.digestEnabled = digestEnabled;
                if (trace)
                    std::cerr << "digest enabled: " << digestEnabled <<
                        std::endl;
            }

            ~StackFrame() {
                ser.digestEnabled = oldVal;
                if (trace)
                    std::cerr << "digest restored: " << oldVal << std::endl;
            }

            T &ser;
            bool oldVal;
        };

        // trace serialization/deesrialization to cerr.
        static bool trace;

        const ModuleDef *module;
        enum DefTypes {
            variableId = 1,
            typeId = 2,
            genericId = 3,
            overloadId = 4,
            aliasId = 5,
            typeAliasId = 6,
            constVarId = 7
        };

        static const int
            modNameSize = 64,
            varNameSize = 16;

        // Digest of all bytes read while 'digestEnabled' is true.
        crack::util::Hasher hasher;

        // if true, every byte we write gets added to the digest.
        bool digestEnabled;

        Serializer(std::ostream &dst) :
            dst(dst),
            module(0),
            objMap(new ObjMap()),
            digestEnabled(false) {
        }

        /** Constructs a nested serializer. */
        Serializer(Serializer &parent, std::ostream &dst) :
            dst(dst),
            module(parent.module),
            objMap(parent.objMap),
            digestEnabled(false) {
        }

        /** Serialize an integer. */
        void write(unsigned int val, const char *name);

        /** Serialize byte data (writes the length followed by the bytes) */
        void write(size_t length, const void *data, const char *name);

        /** Convenience method for writing strings. */
        void write(const std::string &str, const char *name) {
            write(str.size(), str.data(), name);
        }

        /**
         * If we have "object" before, serialize its id with the "definition"
         * flag set to false and return false.  Otherwise serialize a new
         * identifier with a definition flag set to true and return true,
         * indicating that the caller should serialize the state of the object.
         */
        bool writeObject(const spug::RCBase *object, const char *name);

        /**
         * Register an object and get its id without trying to serialize it.
         * This lets us register implicit objects like the current module.
         * Returns the object id.  If the object is already registered, just
         * returns the existing id.
         */
        int registerObject(const spug::RCBase *object);

        /**
         * Returns the id for the previously registered object.
         * Returns -1 if not defined.
         */
        int getObjectId(const spug::RCBase *object) const;

        /**
         * Write a double-precision IEEE float.  These are expected to be 8
         * bytes.
         */
        void writeDouble(double val, const char *name);
};

}

#endif
