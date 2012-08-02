// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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

