// Copyright 2010 Google Inc.

#include "CrackContext.h"

#include <list>
#include <sstream>
#include "parser/Toker.h"
#include "model/Context.h"
#include "model/Namespace.h"
#include "model/PrimFuncAnnotation.h"
#include "Token.h"

using namespace std;
using namespace compiler;
using namespace model;

CrackContext::CrackContext(parser::Parser *parser, parser::Toker *toker,
                           Context *context,
                           void *userData
                           ) :
    parser(parser),
    toker(toker),
    context(context),
    userData(userData) {
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

int CrackContext::getScope() {
    return context->scope;
}

void CrackContext::storeAnnotation(const char *name, AnnotationFunc func) {
    context->compileNS->addDef(new PrimFuncAnnotation(name, func));
}

void CrackContext::storeAnnotation(const char *name, AnnotationFunc func,
                                   void *userData
                                   ) {
    context->compileNS->addDef(new PrimFuncAnnotation(name, func,
                                                      userData
                                                      )
                               );
}

void *CrackContext::getUserData() {
    return userData;
}

