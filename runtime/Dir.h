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

// mirrored in crack
typedef struct {
    const char* name;
    int type;
} _crack_dirEntry;

// opaque to crack
typedef struct {
    DIR* stream;
    dirent* lowLevelEntry;
    _crack_dirEntry currentEntry;
} _crackDir;

// exported interface
_crackDir* _crack_opendir(const char* name);
_crack_dirEntry* _crack_getDirEntry(_crackDir* d);
int _crack_closedir(_crackDir* d);
int _crack_readdir(_crackDir* d);

int _crack_fnmatch(const char* pattern, const char* string); 

}

#endif // _runtime_Dir_h_
