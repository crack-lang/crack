// Copyright 2010 Google Inc.

#include "CrackContext.h"

#include <list>
#include <sstream>
#include "parser/Toker.h"
#include "Token.h"

using namespace std;
using namespace compiler;
using namespace model;

CrackContext::CrackContext(parser::Parser *parser, parser::Toker *toker,
                           model::Context *context
                           ) :
    parser(parser),
    toker(toker),
    context(context) {
}

void CrackContext::inject(char *code) {
    istringstream iss(code);
    parser::Toker tempToker(iss, "injected");
    list<parser::Token> tokens;
    parser::Token tok;
    while (!(tok = tempToker.getToken()).isEnd())
        tokens.push_front(tok);
    
    // transfer the tokens to the tokenizer
    while (!tokens.empty()) {
        toker->putBack(tokens.front());
        tokens.pop_front();
    }
}

Token *CrackContext::getToken() {
    return new Token(toker->getToken());
}

void CrackContext::putBack(Token *tok) {
    toker->putBack(*tok->rep);
}