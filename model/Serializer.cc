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

bool Serializer::writeObject(const void *object, const char *name) {
    ObjMap::iterator iter = objMap.find(object);
    if (iter == objMap.end()) {

        // new object
        if (trace)
            cerr << "writing new object " << name << endl;
        int id = lastId++;
        objMap[object] = id;
        write(id << 1 | 1, "objectId");
        return true;
    } else {
        if (trace)
            cerr << "writing existing object " << name << endl;
        write(iter->second << 1, "objectId");
        return false;
    }
}
