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
#include <sys/time.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <fcntl.h>

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
int float_str(double d, char* buf, unsigned int size) {
    return snprintf(buf, size, "%f", d);
}

int crk_puts(char *str) {
    return puts(str);
}

int crk_putc(char byte) {
    return putchar(byte);
}

void crk_die(const char *message) {
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

bool fileExists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int setNonBlocking(int fd, int val) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return -1;
    if (val)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return fcntl(fd, F_SETFL, flags);
}

int setUtimes(const char *path,
                         int64_t atv_usecs,
                         int64_t mtv_usecs,
                         bool now
                         ) {
    if (!path){
        errno = ENOENT;
        return -1;
    }

    if (now)
        return utimes(path, NULL);

    struct timeval times[2];
    int retval;

    times[0].tv_sec = atv_usecs/1000000;
    times[0].tv_usec = atv_usecs%1000000;

    times[1].tv_sec = atv_usecs/1000000;
    times[1].tv_usec = atv_usecs%1000000;

    return utimes(path, times);
}


}}
