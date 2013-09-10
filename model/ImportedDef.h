// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ImportedDef_h_
#define _model_ImportedDef_h_

#include <string>
#include <vector>

namespace model {

class Deserializer;
class Serializer;

// stores the names for an imported definition.
struct ImportedDef {
    // the "local name" is the alias that a symbol is imported under.
    // the "source name" is the unqualified name that the definition has in
    // the module we are importing it from.
    std::string local, source;

    ImportedDef(const std::string &local, const std::string &source) :
        local(local),
        source(source) {
    }

    /**
     * Initialize both the local and source names to "name".
     */
    ImportedDef(const std::string &name) :
        local(name),
        source(name) {
    }

    void serialize(Serializer &serializer) const;
    static ImportedDef deserialize(Deserializer &deser);
};

typedef std::vector<ImportedDef> ImportedDefVec;

} // namespace model

#endif
