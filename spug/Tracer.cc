// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Tracer.h"

#include <iostream>

using namespace spug;
using namespace std;

namespace {

typedef map<string, Tracer> TracerMap;

// this approach guarantees that the map will be available during static
// initialization.
TracerMap &getTracerMap() {
    static TracerMap map;
    return map;
}

} // anon namespace

Tracer::Tracer(const std::string &name, bool &flag,
               const std::string &desc
               ) :
    flag(flag),
    desc(desc) {

    getTracerMap().insert(make_pair(name, *this));
}

bool Tracer::parse(const char *flags) {
    const char *cur = flags;
    while (*cur) {

        // find the first comma or end of string
        while (*cur && *cur != ',')
            ++cur;

        // find that name in the map
        string name(flags, cur - flags);
        TracerMap &tracers = getTracerMap();
        TracerMap::iterator i = tracers.find(name);
        if (i == tracers.end()) {
            cerr << "Invalid tracer " << name << " found." << endl;
            return false;
        } else {
            i->second.flag = true;
        }

        // restart after the comma
        if (*cur) {
            ++cur;
            flags = cur;
        }
    }
    return true;
}

map<string, string> Tracer::getTracers() {
    map<string, string> results;
    TracerMap &tracers = getTracerMap();
    for (TracerMap::iterator i = tracers.begin();
         i != tracers.end();
         ++i
         )
        results.insert(make_pair(i->first, i->second.desc));
    return results;
}
