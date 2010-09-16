// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include <iostream>

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <errno.h>
#include <string.h>

#include "Dir.h"

extern "C" {

// XXX see http://womble.decadent.org.uk/readdir_r-advisory.html
// for a warning on using readdir_r and how to calculate len
_crackDir* _crack_opendir(const char* name) {
    DIR* d = opendir(name);
    if (!d)
        return NULL;
    _crackDir* cd = (_crackDir*)malloc(sizeof(_crackDir));
    assert(cd && "bad malloc");
    cd->stream = d;
    // this len calculation courtesy of linux readdir manpage
    size_t len = offsetof(dirent, d_name) +
                 pathconf(name, _PC_NAME_MAX) + 1;
    cd->lowLevelEntry = (dirent*)malloc(len);
    assert(cd->lowLevelEntry && "bad malloc");
    return cd;
}

_crack_dirEntry* _crack_getDirEntry(_crackDir* d) {
    assert(d && "null dir pointer");
    return &d->currentEntry;
}

int _crack_closedir(_crackDir* d) {
    assert(d && "null dir pointer");
    closedir(d->stream);
    free(d->lowLevelEntry);
    free(d);
}

int _crack_readdir(_crackDir* d) {

    assert(d && "null dir pointer");
    
    dirent* result;
    int r = readdir_r(d->stream, d->lowLevelEntry, &result);
    assert(!r && "error in readdir");

    // end of stream
    if (!result)
        return 0;

    // TODO make portable
    d->currentEntry.name = d->lowLevelEntry->d_name;

    if (d->lowLevelEntry->d_type == DT_DIR)
        d->currentEntry.type = CRACK_DTYPE_DIR;
    else if (d->lowLevelEntry->d_type == DT_REG)
        d->currentEntry.type = CRACK_DTYPE_FILE;
    else
        d->currentEntry.type = CRACK_DTYPE_OTHER;

    return 1;
}

int _crack_fnmatch(const char* pattern, const char* string) {
    return fnmatch(pattern, string, 0);
}

char* _crack_strerror(void) {
    return strerror(errno);
}

}
