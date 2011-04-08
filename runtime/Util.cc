// Runtime support
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include "Util.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

void puts(char *str) {
    ::puts(str);
}

void __die(const char *message) {
    std::cout << message << std::endl;
    abort();
}

void printfloat(float val) {
    std::cout << val << std::flush;
}

void printint(int val) {
    std::cout << val << std::flush;
}

void printint64(int64_t val) {
    std::cout << val << std::flush;
}

void printuint64(uint64_t val) {
    std::cout << val << std::flush;
}

int is_file(const char *path) {
    struct stat sb;
    if (stat(path, &sb) == -1)
        return 0;
    return S_ISREG(sb.st_mode);
}

}}
