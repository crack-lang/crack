// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_compiler_Token_h_
#define _crack_compiler_Token_h_

#include <stddef.h>
#include "ext/RCObj.h"
#include "init.h"

namespace crack { namespace ext {
    class Module;
}}

namespace parser {
    class Token;
}

namespace compiler {

class Location;

class Token : public crack::ext::RCObj {
    friend void compiler::init(crack::ext::Module *mod);
    private:
        static bool _hasText(Token *inst, const char *text);
        static const char *_getText(Token *inst);
        static int _getType(Token *inst);
        static size_t _getTextSize(Token *inst);
        static Location *_getLocation(Token *inst);
        static void _bind(Token *inst);
        static void _release(Token *inst);
        static bool _isAlias(Token *inst);
        static bool _isAnn(Token *inst);
        static bool _isBoolAnd(Token *inst);
        static bool _isBoolOr(Token *inst);
        static bool _isCase(Token *inst);
        static bool _isCatch(Token *inst);
        static bool _isConst(Token *inst);
        static bool _isIf(Token *inst);
        static bool _isImport(Token *inst);
        static bool _isIn(Token *inst);
        static bool _isElse(Token *inst);
        static bool _isLambda(Token *inst);
        static bool _isModule(Token *inst);
        static bool _isOper(Token *inst);
        static bool _isOn(Token *inst);
        static bool _isWhile(Token *inst);
        static bool _isReturn(Token *inst);
        static bool _isSwitch(Token *inst);
        static bool _isThrow(Token *inst);
        static bool _isTry(Token *inst);
        static bool _isBreak(Token *inst);
        static bool _isClass(Token *inst);
        static bool _isContinue(Token *inst);
        static bool _isDollar(Token *inst);
        static bool _isNull(Token *inst);
        static bool _isIdent(Token *inst);
        static bool _isString(Token *inst);
        static bool _isIstrBegin(Token *inst);
        static bool _isIstrEnd(Token *inst);
        static bool _isSemi(Token *inst);
        static bool _isComma(Token *inst);
        static bool _isColon(Token *inst);
        static bool _isDecr(Token *inst);
        static bool _isDefine(Token *inst);
        static bool _isDot(Token *inst);
        static bool _isIncr(Token *inst);
        static bool _isAssign(Token *inst);
        static bool _isLParen(Token *inst);
        static bool _isRParen(Token *inst);
        static bool _isLCurly(Token *inst);
        static bool _isRCurly(Token *inst);
        static bool _isLBracket(Token *inst);
        static bool _isRBracket(Token *inst);
        static bool _isInteger(Token *inst);
        static bool _isFloat(Token *inst);
        static bool _isOctal(Token *inst);
        static bool _isHex(Token *inst);
        static bool _isBinary(Token *inst);
        static bool _isPlus(Token *inst);
        static bool _isQuest(Token *inst);
        static bool _isMinus(Token *inst);
        static bool _isAsterisk(Token *inst);
        static bool _isBang(Token *inst);
        static bool _isSlash(Token *inst);
        static bool _isPercent(Token *inst);
        static bool _isNot(Token *inst);
        static bool _isTilde(Token *inst);
        static bool _isGT(Token *inst);
        static bool _isLT(Token *inst);
        static bool _isEQ(Token *inst);
        static bool _isNE(Token *inst);
        static bool _isGE(Token *inst);
        static bool _isLE(Token *inst);
        static bool _isEnd(Token *inst);
        static bool _isLogicAnd(Token *inst);
        static bool _isLogicOr(Token *inst);
        static bool _isScoping(Token *inst);
        static bool _isBinOp(Token *inst);
        static bool _isAugAssign(Token *inst);
        
        Token(const Token &other);
        
    public:
        parser::Token *rep;
        Location *loc;

        Token(const parser::Token &tok);
        Token(int type, const char *text, Location *loc);
        ~Token();
        
        static Token *create(int type, const char *text, Location *loc);

        /**
         * Returns true if the token's text form is the same as the string 
         * specified.
         */
        bool hasText(const char *text);
        
        /**
         * Returns the text of the token.  For a string token, this will 
         * return the string contents.
         */
        const char *getText();
        
        /**
         * Returns the token type.
         */
        int getType();
        
        /**
         * Returns the total size of the text in bytes.
         */        
        size_t getTextSize();

        Location *getLocation();

        bool isAlias();
        bool isAnn();
        bool isBoolAnd();
        bool isBoolOr();
        bool isCase();
        bool isCatch();
        bool isConst();
        bool isIf();
        bool isImport();
        bool isIn();
        bool isElse();
        bool isLambda();
        bool isModule();
        bool isOper();
        bool isOn();
        bool isWhile();
        bool isReturn();
        bool isSwitch();
        bool isThrow();
        bool isTry();
        bool isBreak();
        bool isClass();
        bool isContinue();
        bool isDollar();
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
        bool isScoping();
        bool isBinOp();
        bool isAugAssign();
};

} // namespace compiler

#endif

