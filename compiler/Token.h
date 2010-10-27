// Copyright 2010 Google Inc.

#ifndef _crack_compiler_Token_h_
#define _crack_compiler_Token_h_

#include "ext/RCObj.h"

namespace parser {
    class Token;
}

namespace compiler {

class Token : public crack::ext::RCObj {
    public:
        parser::Token *rep;

        Token(const parser::Token &tok);
        ~Token();

        /**
         * Returns true if the token's text form is the same as the string 
         * specified.
         */
        bool hasText(const char *text);

        bool isAnn();
        bool isBoolAnd();
        bool isBoolOr();
        bool isIf();
        bool isImport();
        bool isElse();
        bool isOper();
        bool isWhile();
        bool isReturn();
        bool isBreak();
        bool isClass();
        bool isContinue();
        bool isNull();
        bool isIdent();
        bool isString();
        bool isIstrBegin();
        bool isIstrEnd();
        bool isSemi();
        bool isComma();
        bool isColon();
        bool isDecr();
        bool isDefine();
        bool isDot();
        bool isIncr();
        bool isAssign();
        bool isLParen();
        bool isRParen();
        bool isLCurly();
        bool isRCurly();
        bool isLBracket();
        bool isRBracket();
        bool isInteger();
        bool isFloat();
        bool isOctal();
        bool isHex();
        bool isBinary();
        bool isPlus();
        bool isQuest();
        bool isMinus();
        bool isAsterisk();
        bool isBang();
        bool isSlash();
        bool isPercent();
        bool isNot();
        bool isTilde();
        bool isGT();
        bool isLT();
        bool isEQ();
        bool isNE();
        bool isGE();
        bool isLE();
        bool isEnd();
        bool isLogicAnd();
        bool isLogicOr();
        bool isBinOp();
        bool isAugAssign();
};

} // namespace compiler

#endif
