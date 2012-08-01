// Copyright 2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// macro that is effectively an assert with iostream output.
#ifndef SPUG_CHECK_H
#define SPUG_CHECK_H

#include <iostream>
#include <stdlib.h>

#define SPUG_CHECK(condition, msg) \
    if (!(condition)) {                                                     \
        std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ <<  \
            ": [" << #condition << "] " << msg << std::endl;                \
        abort();                                                            \
    }

#endif
