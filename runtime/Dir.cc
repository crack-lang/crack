// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include <iostream>

#include <assert.h>
#include <stdlib.h>
#include <dirent.h>
#include <fnmatch.h>

#include "Dir.h"

extern "C" {
    
DIR* _crack_opendir(const char* name) {
    return opendir(name);
}

int _crack_closedir(DIR* d) {    
    assert(d && "null dir pointer");
    return closedir(d);
}

int _crack_readdir(DIR* d, _crack_dirEntry* i) {

    assert(d && "null dir pointer");
    assert(i && "null dirEntry pointer");
    
    dirent *e = readdir(d);
    if (!e)
        return 0;

    // TODO make portable
    i->name = e->d_name;

    if (e->d_type == DT_DIR)
        i->type = CRACK_DTYPE_DIR;
    else if (e->d_type == DT_REG)
        i->type = CRACK_DTYPE_FILE;
    else
        i->type = CRACK_DTYPE_OTHER;


    return 1;
}

bool _crack_fnmatch(const char* pattern, const char* string) {
    return fnmatch(pattern, string, 0);
}

}
