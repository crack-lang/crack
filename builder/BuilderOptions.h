// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Conrad Steenberg <conrad.steenberg@gmail.com>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_BuilderOptions_h_
#define _builder_BuilderOptions_h_

#include <spug/RCPtr.h>
#include <spug/RCBase.h>

#include <string>
#include <map>

namespace builder {

SPUG_RCPTR(BuilderOptions);

class BuilderOptions : public spug::RCBase {
    public:

        typedef std::map<std::string, std::string> StringMap;

        // Builder specific optimization aggresiveness
        int optimizeLevel;
        // Builder specific verbosity
        int verbosity;
        // No extra output. Implies verbosity=0
        bool quiet;
        // Dump IR rather than execute/compile code
        bool dumpMode;
        // Generate debug information
        bool debugMode;
        // Keep compile time statistics
        bool statsMode;
        // builder specific option strings
        StringMap optionMap;

        BuilderOptions(void): optimizeLevel(0),
                              verbosity(0),
                              quiet(false),
                              dumpMode(false),
                              debugMode(false),
                              statsMode(false),
                              optionMap() { }

};

} // namespace builder

#endif

