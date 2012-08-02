// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2010 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "ParseError.h"
#include "Token.h"
#include "sstream"

using namespace std;
using namespace parser;

std::string ParseError::getMessage() const {
    stringstream text;
    text << loc << ": " << msg;
    return text.str();
}

void ParseError::abort(const Token &tok, const char *msg) {
   throw ParseError(tok.getLocation(), msg);
}
