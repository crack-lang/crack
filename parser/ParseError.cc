// Copyright 2003 Michael A. Muller

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
