// Copyright 2011 Google Inc.
// Debug tools functions needed by native binary runtimes.

#ifdef GOT_CWD
#include <libcwd/sys.h>  // libcwd requires us to include this before all else
#endif

#include "DebugTools.h"

#ifdef GOT_CWD
#define CWDEBUG
#include <libcwd/debug.h>
#endif

void crack::debug::getLocation(void *address, const char *info[3]) {
#ifdef GOT_CWD
    libcwd::location_ct loc(reinterpret_cast<char *>(address));
    info[0] = loc.mangled_function_name();
    if (loc.is_known()) {
        info[1] = loc.file().c_str();
        info[2] = reinterpret_cast<const char *>(loc.line());
    } else {
        info[1] = "unknown";
        info[2] = 0;
    }
#else
    info[0] = "unknown";
    info[1] = "unknown";
    info[2] = 0;
#endif
}
