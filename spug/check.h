// Copyright 2012 Google Inc.
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
