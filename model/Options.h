// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_Options_h_
#define _model_Options_h_

namespace model {

struct Options {
    // if true, emit warnings about things that have changed since the
    // last version of the language.
    bool migrationWarnings;

    // if true, store modules to a disk cache when compiling and load them
    // from the cache instead of compiling if they're present and up to
    // date.
    bool cacheMode;

    Options() : migrationWarnings(false), cacheMode(true) {}

    // copy the options from another Options object.  This is useful because
    // we typically inherit this struct.
    void copyOptions(const Options &other) {
        *this = other;
    }
};

} // namespace model

#endif
