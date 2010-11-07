// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "Util.h"

namespace crack { namespace runtime {

static bool rseeded = false;
    
char* strerror(void) {
    return ::strerror(errno);
}

// note, not to be used for security purposes
unsigned int rand(unsigned int low, unsigned int high) {
    if (!rseeded) {
        srand(time(NULL));
        rseeded = true;
    }
    int r = ::rand() % (high-low+1)+low;
    return r;
}

// this is temporary until we implement float printing in crack
// assumes caller allocates and owns buffer
void float_str(double d, char* buf, unsigned int size) {
    snprintf(buf, size, "%f", d);
}

}}
