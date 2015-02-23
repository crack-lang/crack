// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Token.h"

#include "parser/Token.h"
#include "Location.h"

using namespace compiler;

Token::Token(const parser::Token &tok) : 
    rep(new parser::Token(tok)),
    loc(0) {
}

Token::Token(int type, const char *text, Location *loc) : loc(loc) {
    rep = new parser::Token(static_cast<parser::Token::Type>(type), text, 
                            *loc->rep
                            );
    loc->bind();
}

Token *Token::create(int type, const char *text, Location *loc) {
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

int Token::getType() {
    return rep->getType();
}

size_t Token::getTextSize() {
    return rep->getData().size();
}

Location *Token::getLocation() {
    if (!loc)
        loc = new Location(rep->getLocation());
    loc->bind();
    return loc;
}

bool Token::_hasText(Token *inst, const char *text) {
    return inst->rep->getData() == text;
}

const char *Token::_getText(Token *inst) {
    return inst->rep->getData().c_str();
}

int Token::_getType(Token *inst) {
    return inst->rep->getType();
}

size_t Token::_getTextSize(Token *inst) {
    return inst->rep->getData().size();
}

Location *Token::_getLocation(Token *inst) {
    if (!inst->loc)
        inst->loc = new Location(inst->rep->getLocation());
    inst->loc->bind();
    return inst->loc;
}

void Token::_bind(Token *inst) { inst->bind(); }
void Token::_release(Token *inst) { inst->release(); }

#define IS_FUNC(name) \
    bool Token::name() { return rep->name(); } \
    bool Token::_##name(Token *inst) { return inst->rep->name(); }

IS_FUNC(isAlias)
IS_FUNC(isAnn)
IS_FUNC(isBoolAnd)
IS_FUNC(isBoolOr)
IS_FUNC(isCase)
IS_FUNC(isCatch)
IS_FUNC(isConst)
IS_FUNC(isIf)
IS_FUNC(isIn)
IS_FUNC(isImport)
IS_FUNC(isElse)
IS_FUNC(isLambda)
IS_FUNC(isModule)
IS_FUNC(isOper)
IS_FUNC(isOn)
IS_FUNC(isWhile)
IS_FUNC(isReturn)
IS_FUNC(isSwitch)
IS_FUNC(isThrow)
IS_FUNC(isTry)
IS_FUNC(isBreak)
IS_FUNC(isClass)
IS_FUNC(isContinue)
IS_FUNC(isDollar)
IS_FUNC(isNull)
IS_FUNC(isIdent)
IS_FUNC(isString)
IS_FUNC(isIstrBegin)
IS_FUNC(isIstrEnd)
IS_FUNC(isSemi)
IS_FUNC(isComma)
IS_FUNC(isColon)
IS_FUNC(isDecr)
IS_FUNC(isDefine)
IS_FUNC(isDot)
IS_FUNC(isIncr)
IS_FUNC(isAssign)
IS_FUNC(isLParen)
IS_FUNC(isRParen)
IS_FUNC(isLCurly)
IS_FUNC(isRCurly)
IS_FUNC(isLBracket)
IS_FUNC(isRBracket)
IS_FUNC(isInteger)
IS_FUNC(isFloat)
IS_FUNC(isOctal)
IS_FUNC(isHex)
IS_FUNC(isBinary)
IS_FUNC(isPlus)
IS_FUNC(isQuest)
IS_FUNC(isMinus)
IS_FUNC(isAsterisk)
IS_FUNC(isBang)
IS_FUNC(isSlash)
IS_FUNC(isPercent)
IS_FUNC(isNot)
IS_FUNC(isTilde)
IS_FUNC(isGT)
IS_FUNC(isLT)
IS_FUNC(isEQ)
IS_FUNC(isNE)
IS_FUNC(isGE)
IS_FUNC(isLE)
IS_FUNC(isEnd)
IS_FUNC(isLogicAnd)
IS_FUNC(isLogicOr)
IS_FUNC(isScoping)
IS_FUNC(isBinOp)
IS_FUNC(isAugAssign)
