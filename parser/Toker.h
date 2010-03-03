// Copyright 2003 Michael A. Muller
// Portions Copyright 2009 Google Inc.

#ifndef TOKER_H
#define TOKER_H

#include <assert.h>
#include <list>
#include <string>
#include "Token.h"
#include "LocationMap.h"

namespace parser {

/** The tokenizer. */
class Toker {
   private:

      // the "put-back" list - where tokens are stored after they've been put
      // back
      std::list<Token> tokens;

      // source stream
      std::istream &src;
      
      // tracks the location
      LocationMap locationMap;

      // "fixes identifiers" by converting them to keywords if appropriate - 
      // if the identifier in 'raw' is a keyword, returns a keyword token, 
      // otherwise just returns the identifier token.
      Token fixIdent(const std::string &raw, const Location &loc);

      // reads the next token directly from the source stream
      Token readToken();

      // info for tracking the state of the tokenizer.
      enum { 
         st_none, 
         st_ident, 
         st_slash,
         st_digram,
         st_comment, 
         st_ccomment,
         st_ccomment2,
         st_string, 
         st_strEscapeChar,
         st_istrEscapeChar,
         st_integer,
         st_istr
      } state;

   public:

      /** constructs a tokenizer from the source stream */
      Toker(std::istream &src, const char *sourceName, int lineNumber = 1);

      /**
       * Returns the next token in the stream.
       */
      Token getToken();

      /**
       * Puts the token back onto the stream.  A subsequent getNext() will
       * return the token.
       */
      void putBack(const Token &token) {
	 tokens.push_back(token);
      }
      
      /**
       * Tells the toker to continue scanning an interpolating string that was 
       * interrupted by a $ sequence.
       */
      void continueIString() {
         assert(state == st_none && "continueIString in invalid state");
         state = st_istr;
      }

};

} // namespace parser

#endif
