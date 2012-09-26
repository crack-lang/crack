// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "config.h"
#include "DebugTools.h"

#include <string.h>
#include <iomanip>
#include <iostream>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
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
    
    struct InternedStringSet : public set<InternedString> {
        ~InternedStringSet() {
            for (InternedStringSet::iterator iter = begin();
                 iter != end();
                 ++iter
                 )
                free(const_cast<char *>(iter->val));
        }
    };

    typedef map<void *, DebugInfo> DebugTable;
    DebugTable debugTable;
    InternedStringSet internTable;

    const InternedString &lookUpString(const InternedString &key) {
        InternedStringSet::iterator iter = internTable.find(key);
        if (iter == internTable.end())
            iter = 
                internTable.insert(InternedString(strdup(key.val))).first;
        return *iter;
    }

    const InternedString &lookUpString(const string &str) {
        InternedString key(str.c_str());
        return lookUpString(key);
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

void crack::debug::registerFuncTable(const char **table) {
    while (table[0]) {
        const InternedString &name = lookUpString(InternedString(table[1]));
        debugTable[(void *)table[0]] = 
            DebugInfo(name.val, "", 0);
        table = table + 2;
    }
}

extern "C" void __CrackRegisterFuncTable(const char **table) {
    crack::debug::registerFuncTable(table);
}

void crack::debug::getLocation(void *address, const char *info[3]) {
    DebugTable::iterator i = debugTable.lower_bound(address);
    if (i == debugTable.end() || i->first != address)
        --i;
    
    if (i == debugTable.end()) {
        info[0] = info[1] = "unknown";
        info[2] = 0;
    } else {
        info[0] = i->second.funcName;
        info[1] = i->second.filename;
        info[2] = reinterpret_cast<const char *>(i->second.lineNumber);
    }
}

void crack::debug::dumpFuncTable(ostream &out) {
    for (DebugTable::iterator i = debugTable.begin(); i != debugTable.end();
         ++i
         )
        out << hex << i->first << " " << i->second.funcName << endl;
}

void *__builtin_frame_address(unsigned int level);

void *crack::debug::getStackFrame() {
    // note: on certain architectures, this won't be able to see past the 
    // current stack frame and will always return zero.
    return __builtin_frame_address(0);
}
