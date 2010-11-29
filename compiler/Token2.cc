// Copyright 2010 Google Inc.

#include "Token.h"

#include "parser/Token.h"
#include "Location.h"

using namespace compiler;

Token::Token(const parser::Token &tok) : 
    rep(new parser::Token(tok)),
    loc(0) {
}

Token::Token(int type, char *text, Location *loc) : loc(loc) {
    rep = new parser::Token(static_cast<parser::Token::Type>(type), text, 
                            *loc->rep
                            );
    loc->bind();
}

Token *Token::create(int type, char *text, Location *loc) {
    return new Token(type, text, loc);
}

Token::~Token() {
    delete rep;
    if (loc)
        loc->release();
}

bool Token::hasText(const char *text) {
    return rep->getData() == text;
}

const char *Token::getText() {
    return rep->getData().c_str();
}

size_t Token::getTextSize() {
    return rep->getData().size();
}

Location *Token::getLocation() {
    if (!loc) {
        loc = new Location(rep->getLocation());
        loc->bind();
    }
    return loc;
}

bool Token::isAnn() { return rep->isAnn(); }
bool Token::isBoolAnd() { return rep->isBoolAnd(); }
bool Token::isBoolOr() { return rep->isBoolOr(); }
bool Token::isIf() { return rep->isIf(); }
bool Token::isIn() { return rep->isIn(); }
bool Token::isImport() { return rep->isImport(); }
bool Token::isElse() { return rep->isElse(); }
bool Token::isOper() { return rep->isOper(); }
bool Token::isOn() { return rep->isOn(); }
bool Token::isWhile() { return rep->isWhile(); }
bool Token::isReturn() { return rep->isReturn(); }
bool Token::isBreak() { return rep->isBreak(); }
bool Token::isClass() { return rep->isClass(); }
bool Token::isContinue() { return rep->isContinue(); }
bool Token::isDollar() { return rep->isDollar(); }
bool Token::isNull() { return rep->isNull(); }
bool Token::isIdent() { return rep->isIdent(); }
bool Token::isString() { return rep->isString(); }
bool Token::isIstrBegin() { return rep->isIstrBegin(); }
bool Token::isIstrEnd() { return rep->isIstrEnd(); }
bool Token::isSemi() { return rep->isSemi(); }
bool Token::isComma() { return rep->isComma(); }
bool Token::isColon() { return rep->isColon(); }
bool Token::isDecr() { return rep->isDecr(); }
bool Token::isDefine() { return rep->isDefine(); }
bool Token::isDot() { return rep->isDot(); }
bool Token::isIncr() { return rep->isIncr(); }
bool Token::isAssign() { return rep->isAssign(); }
bool Token::isLParen() { return rep->isLParen(); }
bool Token::isRParen() { return rep->isRParen(); }
bool Token::isLCurly() { return rep->isLCurly(); }
bool Token::isRCurly() { return rep->isRCurly(); }
bool Token::isLBracket() { return rep->isLBracket(); }
bool Token::isRBracket() { return rep->isRBracket(); }
bool Token::isInteger() { return rep->isInteger(); }
bool Token::isFloat() { return rep->isFloat(); }
bool Token::isOctal() { return rep->isOctal(); }
bool Token::isHex() { return rep->isHex(); }
bool Token::isBinary() { return rep->isBinary(); }
bool Token::isPlus() { return rep->isPlus(); }
bool Token::isQuest() { return rep->isQuest(); }
bool Token::isMinus() { return rep->isMinus(); }
bool Token::isAsterisk() { return rep->isAsterisk(); }
bool Token::isBang() { return rep->isBang(); }
bool Token::isSlash() { return rep->isSlash(); }
bool Token::isPercent() { return rep->isPercent(); }
bool Token::isNot() { return rep->isNot(); }
bool Token::isTilde() { return rep->isTilde(); }
bool Token::isGT() { return rep->isGT(); }
bool Token::isLT() { return rep->isLT(); }
bool Token::isEQ() { return rep->isEQ(); }
bool Token::isNE() { return rep->isNE(); }
bool Token::isGE() { return rep->isGE(); }
bool Token::isLE() { return rep->isLE(); }
bool Token::isEnd() { return rep->isEnd(); }
bool Token::isLogicAnd() { return rep->isLogicAnd(); }
bool Token::isLogicOr() { return rep->isLogicOr(); }
bool Token::isBinOp() { return rep->isBinOp(); }
bool Token::isAugAssign() { return rep->isAugAssign(); }
