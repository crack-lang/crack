// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Dir_h_
#define _runtime_Dir_h_

#include <dirent.h>

extern "C" {

// needs to match dir.crk in runtime
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

int _crack_fnmatch(const char* pattern, const char* string); 

}

#endif // _runtime_Dir_h_
