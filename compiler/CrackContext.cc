// Copyright 2010 Google Inc.

#include "CrackContext.h"

#include <stdlib.h>
#include <list>
#include <sstream>
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/Context.h"
#include "model/Namespace.h"
#include "model/PrimFuncAnnotation.h"
#include "Token.h"

using namespace std;
using namespace compiler;
using namespace model;

namespace {
    // Implements parser callback to wrap raw functions.
    struct Callback : parser::ParserCallback {
        parser::Parser::Event event;
        CrackContext::AnnotationFunc func;

        Callback(parser::Parser::Event event,
                 CrackContext::AnnotationFunc func
                 ) :
            event(event),
            func(func) {
        }

        virtual void run(parser::Parser *parser, parser::Toker *toker, 
                        model::Context *context
                        ) {
            CrackContext ctx(parser, toker, context);
            func(&ctx);
        }
    };
}

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

void CrackContext::error(const char *text) {
    const parser::Location &loc = context->getLocation();
    cerr << "ParseError: " << loc.getName() << ':' << loc.getLineNumber() << 
        ": " << text << endl;
    exit(1);
}

void CrackContext::error(Token *tok, const char *text) {
    const parser::Location &loc = tok->rep->getLocation();
    cerr << "ParseError: " << loc.getName() << ':' << loc.getLineNumber() << 
        ": " << text << endl;
    exit(1);
}

void CrackContext::warn(const char *text) {
    parser::Parser::warn(context->getLocation(), text);
}

void CrackContext::warn(Token *tok, const char *text) {
    parser::Parser::warn(tok->rep->getLocation(), text);
}

int CrackContext::getParseState() {
    return parser->state;
}

parser::ParserCallback *CrackContext::addCallback(
    int event,
    CrackContext::AnnotationFunc func
    ) {
    parser::Parser::Event evt = static_cast<parser::Parser::Event>(event);
    Callback *callback = new Callback(evt, func);
    parser->addCallback(evt, callback);
    return callback;
}

void CrackContext::removeCallback(parser::ParserCallback *callback) {
    Callback *cb = dynamic_cast<Callback *>(callback);
    parser::Parser::Event event =
        static_cast<parser::Parser::Event>(cb->event);
    if (parser->removeCallback(event, cb))
        delete callback;
    else
        error("Attempted to remove a callback that wasn't registered.");
}

void CrackContext::setNextFuncFlags(int nextFuncFlags) {
    context->nextFuncFlags = static_cast<FuncDef::Flags>(nextFuncFlags);
}
