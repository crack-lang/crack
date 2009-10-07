
#ifndef TOKEN_H
#define TOKEN_H

#include "Location.h"

namespace parser {

/** Basic representation of a token. */
class Token {
   public:

      // the token types
      typedef enum { ident, string, semi, comma, colon, dot, assign, lparen, 
		     rparen, lcurly, rcurly, oper, integer, plus, minus, 
		     asterisk, slash, end
		    } Type;

   private:

      // the token's state
      Type type;
      std::string data;

      // source location for the token
      Location loc;

   public:

      Token();

      Token(Type type, const char *data, const Location &loc);

      /** returns the token type */
      Type getType() const { return type; }

      /** returns the token raw data */
      const std::string &getData() const { return data; }

      /** Returns the source location for the token */
      const Location &getLocation() const { return loc; }

      /** dump a representation of the token to a stream */
      friend std::ostream &operator <<(std::ostream &out, const Token &tok) {
	 out << tok.loc << ":\"" << tok.data;
      }

      /** Methods to check the token type */
      /** @{ */

      bool isIdent() const { return type == ident; }
      bool isString() const { return type == string; }
      bool isSemi() const { return type == semi; }
      bool isComma() const { return type == comma; }
      bool isColon() const { return type == colon; }
      bool isDot() const { return type == dot; }
      bool isAssign() const { return type == assign; }
      bool isLParen() const { return type == lparen; }
      bool isRParen() const { return type == rparen; }
      bool isLCurly() const { return type == lcurly; }
      bool isRCurly() const { return type == rcurly; }
      bool isInteger() const { return type == integer; }
      bool isPlus() const { return type == plus; }
      bool isMinus() const { return type == minus; }
      bool isAsterisk() const { return type == asterisk; }
      bool isSlash() const { return type == slash; }
      bool isEnd() const { return type == end; }

      /** @} */

};

} // namespace parser

#endif
