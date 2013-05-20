// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_NestedDeserializer_h_
#define _model_NestedDeserializer_h_

#include <string>
#include <sstream>

#include "Deserializer.h"

namespace model {

/**
 * This is a convenience class that bundles together a Deserializer the string
 * and stringstream needed to support it.  When you create one of these, it
 * reads its data as a string from the parent Deserializer and creates the
 * whole family of objects.
 */
class NestedDeserializer : public Deserializer {
    private:
        std::string data;
        std::istringstream src;

    public:
        NestedDeserializer(Deserializer &parent, size_t expectedMaxSize,
                           const char *name
                           ) :
            Deserializer(parent, src),
            data(parent.readString(expectedMaxSize, name)),
            src(data) {
        }
};

} // namespace model

#endif
