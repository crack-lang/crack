// Copyright 2010 Google Inc.

#ifndef _crack_compiler_Location_h_
#define _crack_compiler_Location_h_

#include "ext/RCObj.h"

namespace parser {
    class Location;
}

namespace compiler {

class Location : public crack::ext::RCObj {
    private:
        Location(const Location &other);

    public:
        parser::Location *rep;

        Location(const parser::Location &loc);
        ~Location();
        
        /**
         * Returns the file name of the location.
         */
        const char *getName();
        
        /**
         * Returns the line number of the location.
         */
        int getLineNumber();
};

} // namespace compiler

#endif

