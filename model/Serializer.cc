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

using namespace spug;
using namespace std;
using namespace model;

bool Serializer::trace = false;

void Serializer::write(unsigned int val, const char *name) {
    if (trace)
        cerr << "write uint " << name << ": " << val << endl;

    // special case 0
    if (!val) {
        dst << static_cast<char>(val);
        return;
    }

    while (val) {
        uint8_t b = val & 0x7f;
        val >>= 7;
        if (val)
            b |= 0x80;
        dst << b;
    }
}

void Serializer::write(size_t length, const void *data, const char *name) {
    write(length, name);
    if (trace)
        cerr << "write blob " << name << ": " << setw(length) <<
            static_cast<const char *>(data) << endl;
    dst.write(reinterpret_cast<const char *>(data), length);
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

void Serializer::writeDouble(double val, const char *name) {
    SPUG_CHECK(sizeof(double) == 8,
               "double != 8 chars on this platform, size is: " <<
                sizeof(double)
               );
    if (trace)
        cerr << "write double " << name << ": " << val << endl;
    dst.write(reinterpret_cast<const char *>(&val), sizeof(double));
}
