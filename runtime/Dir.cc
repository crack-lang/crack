// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

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

const char* _crack_readdir(DIR* d) {
    dirent *e = readdir(d);
    return e->d_name;
}


}
