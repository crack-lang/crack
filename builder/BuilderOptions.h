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

        typedef std::map<std::string, std::string> stringMap;

        // Builder specific optimization aggresiveness
        int optimizeLevel;
        // Builder specific verbosity
        int verbosity;
        // Dump IR rather than execute/compile code
        bool dumpMode;
        // Generate debug information
        bool debugMode;
        // builder specific option strings
        stringMap optionMap;

};

} // namespace builder

#endif

