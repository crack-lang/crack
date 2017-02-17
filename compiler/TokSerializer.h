// Copyright 2017 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _crack_compiler_TokSerializer_h_
#define _crack_compiler_TokSerializer_h_

#include <stdlib.h>

#include <list>

#include "parser/Location.h"
#include "ext/RCObj.h"

namespace compiler {

class Location;
class Token;

// TokSerializer is a FIFO of tokens that can be used to produce a serialized
// string representation of the tokens and can also be constructed from a
// serialized string, allowing you to retrieve the tokens.
// This is used to represent a token list as a string constant, which is
// useful for building macros within an annotation.
class TokSerializer : public crack::ext::RCObj {
    private:
        std::list<Token *> tokens;
        Location *loc;

    public:
        TokSerializer() : loc(0) {}
        ~TokSerializer();

        // Create a serializer from a string.
        static TokSerializer *create(const char *serialized, size_t size);

        // Create an empty serializer.
        static TokSerializer *create();

        // Insert a new token at the head of the list.
        static void insert(TokSerializer *inst, Token *tok);

        // Get the next token.
        static Token *getToken(TokSerializer *inst);

        // Serialize the token to a string.  Caller takes ownership of the
        // resulting memory.
        static char *serialize(TokSerializer *inst);

        // Sets the location associated with the serializer.
        static void setLocation(TokSerializer *inst, Location *loc);

        // Returns the location set with setLocation().
        static Location *getLocation(TokSerializer *inst);
};

}  // namespace compiler

#endif
