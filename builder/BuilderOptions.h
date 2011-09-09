// Copyright 2009-2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

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
        // Dump IR rather than execute/compile code
        bool dumpMode;
        // Generate debug information
        bool debugMode;
        // Keep compile time statistics
        bool statsMode;
        // Enable builder module caching
        bool cacheMode;
        // builder specific option strings
        StringMap optionMap;

        BuilderOptions(void): optimizeLevel(0),
                              verbosity(0),
                              dumpMode(false),
                              debugMode(false),
                              statsMode(false),
                              cacheMode(true),
                              optionMap() { }

};

} // namespace builder

#endif

