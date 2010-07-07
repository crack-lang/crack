// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include <stdlib.h>
#include <iostream>
#include <dirent.h>
#include "Dir.h"

extern "C" {
    
DIR* _crack_opendir(const char* name) {
    return opendir(name);
}

int _crack_closedir(DIR* d) {    
    return closedir(d);
}

int _crack_readdir(DIR* d, _crack_dirEntry* i) {

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


}
