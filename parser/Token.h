// Copyright 2003 Michael A. Muller
// Copyright 2009 Google Inc.

#ifndef TOKEN_H
#define TOKEN_H

#include "Location.h"

namespace parser {

/** Basic representation of a token. */
class Token {
   public:

      // the token types
      typedef enum { ann, bitAnd, bitLSh, bitOr, bitRSh, bitXor, breakKw, 
                     classKw, continueKw, elseKw, ifKw, importKw, isKw, 
                     nullKw, operKw, returnKw, whileKw, assign, assignAnd, 
                     assignAsterisk, assignLSh, assignOr, assignRSh, 
                     assignXor, assignMinus, assignPercent, assignPlus, 
                     assignSlash, asterisk, bang, colon, comma, decr, define, 
                     dot, end, eq, ge, gt, ident, incr, integer, lbracket, 
                     lcurly, le, lparen, lt, minus, ne, percent, plus, quest, 
                     rbracket, rcurly, rparen, semi, slash, string, tilde, 
                     istrBegin, istrEnd, logicAnd, logicOr, floatLit, 
                     octalLit, hexLit, binLit
		    } Type;

   private:

      // the token's state
      Type type;
      std::string data;

      // source location for the token
      Location loc;

   public:

      Token();

      Token(Type type, const std::string &data, const Location &loc);

      /** returns the token type */
      Type getType() const { return type; }

      /** returns the token raw data */
      const std::string &getData() const { return data; }

      /** Returns the source location for the token */
      const Location &getLocation() const { return loc; }

      /** dump a representation of the token to a stream */
      friend std::ostream &operator <<(std::ostream &out, const Token &tok) {
	 return out << tok.loc << ":\"" << tok.data;
      }

      /** Methods to check the token type */
      /** @{ */

      bool isAnn() const { return type == ann; }
      bool isBoolAnd() const { return type == bitAnd; }
      bool isBoolOr() const { return type == bitOr; }
      bool isIf() const { return type == ifKw; }
      bool isImport() const { return type == importKw; }
      bool isElse() const { return type == elseKw; }
      bool isOper() const { return type == operKw; }
      bool isWhile() const { return type == whileKw; }
      bool isReturn() const { return type == returnKw; }
      bool isBreak() const { return type == breakKw; }
      bool isClass() const { return type == classKw; }
      bool isContinue() const { return type == continueKw; }
      bool isNull() const { return type == nullKw; }
      bool isIdent() const { return type == ident; }
      bool isString() const { return type == string; }
      bool isIstrBegin() const { return type == istrBegin; }
      bool isIstrEnd() const { return type == istrEnd; }
      bool isSemi() const { return type == semi; }
      bool isComma() const { return type == comma; }
      bool isColon() const { return type == colon; }
      bool isDecr() const { return type == decr; }
      bool isDefine() const { return type == define; }
      bool isDot() const { return type == dot; }
      bool isIncr() const { return type == incr; }
      bool isAssign() const { return type == assign; }
      bool isLParen() const { return type == lparen; }
      bool isRParen() const { return type == rparen; }
      bool isLCurly() const { return type == lcurly; }
      bool isRCurly() const { return type == rcurly; }
      bool isLBracket() const { return type == lbracket; }
      bool isRBracket() const { return type == rbracket; }
      bool isInteger() const { return type == integer; }
      bool isFloat() const { return type == floatLit; }
      bool isOctal() const { return type == octalLit; }
      bool isHex() const { return type == hexLit; }
      bool isBinary() const { return type == binLit; }
      bool isPlus() const { return type == plus; }
      bool isQuest() const { return type == quest; }
      bool isMinus() const { return type == minus; }
      bool isAsterisk() const { return type == asterisk; }
      bool isBang() const { return type == bang; }
      bool isSlash() const { return type == slash; }
      bool isPercent() const { return type == percent; }
      bool isNot() const { return type == bang; }
      bool isTilde() const { return type == tilde; }
      bool isGT() const { return type == gt; }
      bool isLT() const { return type == lt; }
      bool isEQ() const { return type == eq; }
      bool isNE() const { return type == ne; }
      bool isGE() const { return type == ge; }
      bool isLE() const { return type == le; }
      bool isEnd() const { return type == end; }
      bool isLogicAnd() const { return type == logicAnd; }
      bool isLogicOr() const { return type == logicOr; }
      
      bool isBinOp() const {
         switch (type) {
            case plus:
            case minus:
            case asterisk:
            case slash:
            case percent:
            case eq:
            case ne:
            case lt:
            case gt:
            case le:
            case ge:
            case isKw:
            case logicAnd:
            case logicOr:
            case bitAnd:
            case bitOr:
            case bitXor:
            case bitLSh:
            case bitRSh:
               return true;
            default:
               return false;
         }
      }

      bool isAugAssign() const {
         switch(type) {
            case assignAnd:
            case assignAsterisk:
            case assignLSh:
            case assignOr:
            case assignRSh:
            case assignXor:
            case assignMinus: 
            case assignPercent:
            case assignPlus:
            case assignSlash:
               return true;
            default:
               return false;
         }
      }

      /** @} */

};

} // namespace parser

#endif
