// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Dir_h_
#define _runtime_Dir_h_

#include <dirent.h>

extern "C" {

#define CRACK_DTYPE_DIR   1
#define CRACK_DTYPE_FILE  2
#define CRACK_DTYPE_OTHER 3

typedef struct {
    char* name;
    int type;
} _crack_dirEntry;

DIR* _crack_opendir(const char* name);
int _crack_closedir(DIR* d);
int _crack_readdir(DIR* d, _crack_dirEntry* i);

}

#endif // _runtime_Dir_h_
