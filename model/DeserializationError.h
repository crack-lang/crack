// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_DeserializationError_h_
#define _model_DeserializationError_h_

#include <spug/Exception.h>

namespace model {

class DeserializationError : public spug::Exception {
    public:
        DeserializationError(const std::string &text) : Exception(text) {}
};

} // namespace model

#endif

