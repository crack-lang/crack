// Runtime support for directory access
// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _runtime_Dir_h_
#define _runtime_Dir_h_

#include <dirent.h>

namespace crack { namespace runtime {

// needs to match dir.crk in runtime
#define CRACK_DTYPE_DIR   1
#define CRACK_DTYPE_FILE  2
#define CRACK_DTYPE_OTHER 3

// mirrored in crack
typedef struct {
    const char* name;
    int type;
} DirEntry;

// opaque to crack
typedef struct {
    DIR* stream;
    dirent* lowLevelEntry;
    DirEntry currentEntry;
} Dir;

// exported interface
Dir* opendir(const char* name);
DirEntry* getDirEntry(Dir* d);
int closedir(Dir* d);
int readdir(Dir* d);

int fnmatch(const char* pattern, const char* string); 

bool Dir_toBool(Dir *dir);
void *Dir_toVoidptr(Dir *dir);

}} // namespace crack::ext

#endif // _runtime_Dir_h_
