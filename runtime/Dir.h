// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#ifndef _runtime_Dir_h_
#define _runtime_Dir_h_

#include <dirent.h>

extern "C" {

DIR* _crack_opendir(const char* name);
int _crack_closedir(DIR* d);

}

#endif // _runtime_Dir_h_
