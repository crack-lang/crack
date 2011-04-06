// Copyright 2011 Google Inc.

#ifdef GOT_CWD
#include <libcwd/sys.h> // libcwd requires us to include this before all else
#endif

#include "DebugTools.h"

#ifdef GOT_CWD
#define CWDEBUG
#include <libcwd/debug.h>
#endif

#include <string.h>
#include <map>
#include <set>

using namespace std;

namespace {
    
    struct DebugInfo {
        const char *funcName;
        const char *filename;
        int lineNumber;
        
        DebugInfo(const char *funcName, const char *filename, int lineNumber) :
            funcName(funcName),
            filename(filename),
            lineNumber(lineNumber) {
        }
        
        DebugInfo() : funcName(0), filename(0), lineNumber(0) {}
    };
    
    struct InternedString {
        const char *val;
    
        InternedString(const char *val) : val(val) {}
        
        bool operator <(const InternedString &other) const {
            return strcmp(val, other.val) < 0;
        }
    };
    
    typedef map<void *, DebugInfo> DebugTable;
    DebugTable debugTable;
    typedef set<InternedString> InternedStringSet;
    InternedStringSet internTable;

    const InternedString &lookUpString(const string &str) {
        InternedString key(str.c_str());
        InternedStringSet::iterator iter = internTable.find(key);
        if (iter == internTable.end())
            iter = 
                internTable.insert(InternedString(strdup(str.c_str()))).first;
        return *iter;
    }
}

void crack::debug::registerDebugInfo(void *address, 
                                     const string &funcName,
                                     const string &fileName,
                                     int lineNumber
                                     ) {
    const InternedString &name = lookUpString(funcName);
    const InternedString &file = lookUpString(fileName);
    debugTable[address] = DebugInfo(name.val, file.val, lineNumber);
}

void crack::debug::getLocation(void *address, const char *info[3]) {
    DebugTable::iterator i = debugTable.lower_bound(address);
    if (i == debugTable.end() || i->first != address)
        --i;
    
    if (i == debugTable.end()) {

#ifdef GOT_CWD
        // try to find the info from the static debug info
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
        info[1] = "unknown";
        info[2] = 0;
#endif
    } else {
        info[0] = i->second.funcName;
        info[1] = i->second.filename;
        info[2] = reinterpret_cast<const char *>(i->second.lineNumber);
    }
}
