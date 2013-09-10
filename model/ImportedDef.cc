// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ImportedDef.h"

#include <sstream>
#include "Deserializer.h"
#include "NestedDeserializer.h"
#include "ProtoBuf.h"
#include "Serializer.h"

using namespace model;
using namespace std;

void ImportedDef::serialize(Serializer &serializer) const {
    serializer.write(source, "sourceName");
    if (local != source) {
        ostringstream tmp;
        Serializer ser(serializer, tmp);
        ser.write(CRACK_PB_KEY(1, string), "localName.header");
        ser.write(local, "localName");
        serializer.write(tmp.str(), "optional");
    } else {
        serializer.write(0, "optional");
    }
}

ImportedDef ImportedDef::deserialize(Deserializer &deser) {
    string source = deser.readString(Serializer::varNameSize, "sourceName");
    string local;
    CRACK_PB_BEGIN(deser, 16, optional)
        CRACK_PB_FIELD(1, string) {
            local = optionalDeser.readString(Serializer::varNameSize, "name");
            break;
        }
    CRACK_PB_END
    if (local.size())
        return ImportedDef(local, source);
    else
        return ImportedDef(source);
}
