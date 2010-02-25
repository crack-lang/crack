// Copyright 2003 Michael A. Muller
// Copyright 2009 Google Inc.

#include "Token.h"

using namespace std;
using namespace parser;

Token::Token() :
   type(Token::end) {
}

Token::Token(Type type, const std::string &data, const Location &loc) :
    type(type),
    data(data),
    loc(loc) {
}

