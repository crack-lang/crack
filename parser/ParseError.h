// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2010 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef PARSEERROR_H
#define PARSEERROR_H

#include <spug/Exception.h>
#include "Location.h"

namespace parser {

class Token;

/**
 * Exception class for parsing errors.
 */
class ParseError : public spug::Exception {
    protected:
        Location loc;
    public:
        ParseError() {}

        ParseError(const Location &loc, const char *msg) :
            spug::Exception(msg),
            loc(loc) {

        }

        ParseError(const Location &loc, const std::string &msg) :
            spug::Exception(msg),
            loc(loc) {

        }

        ~ParseError() throw () {}

        virtual const char *getClassName() const { return "ParseError"; }
        
        virtual std::string getMessage() const;

        static void abort(const Token &tok, const char *msg);

};

} // namespace parser

#endif
