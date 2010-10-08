// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "Util.h"

extern "C" {

char* _crack_strerror(void) {
    return strerror(errno);
}

// this is temporary until we implement float printing in crack
// assumes caller allocates and owns buffer
void _crack_float_str(double d, char* buf, unsigned int size) {
    snprintf(buf, size, "%f", d);
}

}
