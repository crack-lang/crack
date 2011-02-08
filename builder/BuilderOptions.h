// Copyright 2009-2011 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#ifndef _builder_BuilderOptions_h_
#define _builder_BuilderOptions_h_

#include <spug/RCPtr.h>
#include <spug/RCBase.h>

namespace builder {

SPUG_RCPTR(BuilderOptions);

class BuilderOptions : public spug::RCBase {
    public:
        // Builder specific optimization aggresiveness
        int optimizeLevel;
        // Builder specific verbosity
        int verbosity;
        // Dump IR rather than execute/compile code
        bool dumpMode;
        // Generate debug information
        bool debugMode;
};

} // namespace builder

#endif

