// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef SPUG_TRACER_H
#define SPUG_TRACER_H

#include <map>
#include <string>

namespace spug {

/**
 * Tracers are use to allow you to turn on tracing for a specific area of
 * functionality.  It is intended for use with a global flag variable.  For
 * example:
 *
 *   bool Foo::trace = false;
 *   Tracer tracer("Foo", &trace, "The foomatic subsystem");
 *
 * Hopefully all of these static tracer instances will get resolved before
 * main(), where you can parsse a flag value like this:
 *
 *   Tracer::parse(argValue);
 *
 * This will set all of the relevant flags.
 */
class Tracer {
    private:
        bool &flag;
        std::string desc;

    public:
        Tracer(const std::string &name, bool &flag,
               const std::string &desc
               );

        /**
         * Parses a string containing a comma separate list of tracer flag
         * names and sets the corresponding flags.  Returns true if all flags
         * were parsed successfully, false if unknown flags were discovered
         * (it write the unknown flag to stderr).
         */
        static bool parse(const char *flags);

        /**
         * Returns a map of "name, desc" pairs for all tracer flags that
         * have been registered.
         */
        static std::map<std::string, std::string> getTracers();

};

};

#endif
