// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Import_h_
#define _model_Import_h_

#include "spug/RCBase.h"
#include "spug/RCPtr.h"

#include "ImportedDef.h"

namespace model {

class Deserializer;
SPUG_RCPTR(Import);
class Serializer;

/**
 * Keeps track of an import statement (currently for purposes of tracking the
 * compile namespace for generics).
 */
class Import : public spug::RCBase {
    public:
        std::vector<std::string> moduleName;
        ImportedDefVec syms;

        Import(const std::vector<std::string> &moduleName,
               const ImportedDefVec &syms
               ) :
            moduleName(moduleName),
            syms(syms) {
        }

        void serialize(Serializer &serializer) const;
        static ImportPtr deserialize(Deserializer &deser);
};

} // namespace model

#endif
