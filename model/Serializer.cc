// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Serializer.h"

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include "spug/check.h"
#include "spug/Tracer.h"

using namespace spug;
using namespace std;
using namespace model;
using crack::util::Hasher;

bool Serializer::trace = false;
namespace {
    spug::Tracer tracer("Serializer", Serializer::trace,
                        "Meta-data serialization and deserialization."
                        );
}

void Serializer::write(unsigned int val, const char *name) {
    if (trace)
        cerr << "write uint " << name << ": " << val << endl;

    // special case 0
    if (!val) {
        dst << static_cast<char>(val);
        if (digestEnabled)
            hasher.add(0);
        return;
    }

    while (val) {
        uint8_t b = val & 0x7f;
        val >>= 7;
        if (val)
            b |= 0x80;
        dst << b;
        if (digestEnabled)
            hasher.add(b);
    }
}

void Serializer::write(size_t length, const void *data, const char *name) {
    write(length, name);
    if (trace)
        cerr << "write blob " << name << ": " << setw(length) <<
            static_cast<const char *>(data) << endl;
    dst.write(reinterpret_cast<const char *>(data), length);
    if (digestEnabled)
        hasher.add(data, length);
}

bool Serializer::writeObject(const RCBase *object, const char *name) {
    ObjMap::iterator iter = objMap->find(object);
    if (iter == objMap->end()) {

        // new object
        if (trace)
            cerr << "writing new object " << name << endl;
        int id = objMap->lastId++;
        (*objMap)[object] = id;
        write(id << 1 | 1, "objectId");
        return true;
    } else {
        if (trace)
            cerr << "writing existing object " << name << endl;
        write(iter->second << 1, "objectId");
        return false;
    }
}

int Serializer::registerObject(const RCBase *object) {
    ObjMap::iterator iter = objMap->find(object);
    if (iter == objMap->end()) {
        int id = objMap->lastId++;
        (*objMap)[object] = id;
        return id;
    } else {
        return iter->second;
    }
}

int Serializer::getObjectId(const RCBase *object) const {
    ObjMap::iterator iter = objMap->find(object);
    if (iter == objMap->end())
        return -1;
    else
        return iter->second;
}

void Serializer::writeDouble(double val, const char *name) {
    SPUG_CHECK(sizeof(double) == 8,
               "double != 8 chars on this platform, size is: " <<
                sizeof(double)
               );
    if (trace)
        cerr << "write double " << name << ": " << val << endl;
    dst.write(reinterpret_cast<const char *>(&val), sizeof(double));
    if (digestEnabled)
        hasher.add(&val, sizeof(double));
}
