// Copyright 2010 Google Inc.

#ifndef _crack_compiler_Location_h_
#define _crack_compiler_Location_h_

#include "ext/RCObj.h"

namespace crack { namespace ext {
    class Module;
}}

namespace parser {
    class Location;
}

namespace compiler {

class Location : public crack::ext::RCObj {
    friend void compiler::init(crack::ext::Module *mod);
    private:
        Location(const Location &other);

        static const char *_getName(Location *inst);
        static int _getLineNumber(Location *inst);
        static void _bind(Location *inst);
        static void _release(Location *inst);

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

