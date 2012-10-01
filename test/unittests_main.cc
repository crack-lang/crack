// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include <sstream>
#include "model/Serializer.cc"
#include "model/Deserializer.cc"

void serializerTestUInt() {
    using namespace model;

    ostringstream dst;
    Serializer s(dst);

    s.write(257);
    string data = dst.str();
    istringstream src(data);

    if (data[0] != 0x82)
        cerr << "first byte of data is " << static_cast<unsigned>(data[0] &
                                                                  0xFF)
             << endl;
    if (data[1] != 1)
        cerr << "second byte of data is " << static_cast<unsigned>(data[1])
             << endl;

    Deserializer d(src);
    int val = d.readUInt();
    if (val != 257)
        cerr << "write/read of uint failed, got " << val << endl;
}

int main(int argc, const char **argv) {
    serializerTestUInt();
}
