
#ifndef TOKER_H
#define TOKER_H

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

};

} // namespace parser

#endif
