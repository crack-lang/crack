// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Deserializer.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include "spug/check.h"
#include "spug/RCBase.h"
#include "Serializer.h"
#include "DeserializationError.h"

using namespace std;
using namespace model;
using crack::util::Hasher;

unsigned int Deserializer::readUInt(const char *name, bool *eof) {
    uint8_t b = 0x80;
    unsigned val = 0, offset = 0;
    while (b & 0x80) {
        if (src.eof()) {
            if (eof) {
                *eof = true;
                return 0;
            } else {
                throw DeserializationError("EOF deserializing meta-data");
            }
        }
        b = src.get();
        if (digestEnabled)
            hasher.add(b);

        // see if we've got the last byte
        val = val | ((b & 0x7f) << offset);
        offset += 7;
    }

    if (Serializer::trace)
        cerr << "read uint " << name << ": " << val << endl;

    if (eof)
        *eof = false;
    return val;
}

char *Deserializer::readBlob(size_t &size, char *buffer, const char *name) {
    size_t cap = size;
    size = readUInt(name);

    if (size > cap || !buffer)
        buffer = new char[size];

    src.read(buffer, size);
    if (src.fail())
        throw DeserializationError("EOF deserializing meta-data");
    if (Serializer::trace)
        cerr << "read blob " << name << ": " << setw(size) << buffer << endl;
    if (digestEnabled)
        hasher.add(buffer, size);
    return buffer;
}

string Deserializer::readString(size_t expectedMaxSize, const char *name) {
    char buffer[expectedMaxSize];
    size_t size = expectedMaxSize;
    memset(buffer, 0, size);
    char *tmp = readBlob(size, buffer, name);
    if (tmp != buffer) {
        string result(tmp, size);
        delete [] tmp;
        return result;
    } else {
        return string(tmp, size);
    }
}

Deserializer::ReadObjectResult Deserializer::readObject(
    const ObjectReader &reader,
    const char *name
) {
    int id = readUInt(name);
    if (id & 1) {
        // this is a definition - let the reader read the object
        if (Serializer::trace)
            cerr << "reading new object " << name << " id = " << id << endl;
        userData = 0;
        spug::RCBasePtr obj = reader.read(*this);
        (*objMap)[id >> 1] = obj;
        return ReadObjectResult(obj, true, userData);
    } else {
        // the object should already exist
        if (Serializer::trace)
            cerr << "reading existing object " << name <<  " id = " << id <<
                endl;
        ObjMap::iterator iter = objMap->find(id >> 1);
        assert(iter != objMap->end() && "Unable to resolve serialized object");
        return ReadObjectResult(iter->second, false, 0);
    }
}

spug::RCBasePtr Deserializer::getObject(int id) const {
    ObjMap::iterator iter = objMap->find(id);
    if (iter == objMap->end())
        return 0;
    else
        return iter->second;
}

void Deserializer::registerObject(int id, spug::RCBase *object) {
    SPUG_CHECK(objMap->find(id) == objMap->end(),
               "The object id " << id << " is already registered."
               );
    (*objMap)[id] = object;
}

double Deserializer::readDouble(const char *name) {
    SPUG_CHECK(sizeof(double) == 8,
               "double != 8 chars on this platform, size is: " <<
                sizeof(double)
               );
    double val;
    src.read(reinterpret_cast<char *>(&val), sizeof(double));
    if (src.fail())
        throw DeserializationError("EOF deserializing meta-data");
    if (Serializer::trace)
        cerr << "reading double " << name << ": " << val << endl;
    if (digestEnabled)
        hasher.add(&val, sizeof(double));
    return val;
}