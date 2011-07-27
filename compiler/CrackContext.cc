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
#include "Annotation.h"
#include "Location.h"

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

void CrackContext::inject(char *sourceName, int lineNumber, char *code) {
    istringstream iss(code);
    parser::Toker tempToker(iss, sourceName, lineNumber);
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

compiler::Annotation *CrackContext::getAnnotation(const char *name) {
    model::Annotation *ann = 
        model::AnnotationPtr::rcast(context->compileNS->lookUp(name));
    return ann ? new Annotation(ann) : 0;
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
    context->error(text, false);
}

void CrackContext::error(Token *tok, const char *text) {
    context->error(tok->rep->getLocation(), text, false);
}

void CrackContext::warn(const char *text) {
    context->warn(text);
}

void CrackContext::warn(Token *tok, const char *text) {
    context->warn(tok->rep->getLocation(), text);
}

int CrackContext::getParseState() {
    return parser->state;
}

void CrackContext::pushErrorContext(const char *text) {
    context->pushErrorContext(text);
}

void CrackContext::popErrorContext() {
    context->popErrorContext();
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

void CrackContext::setNextClassFlags(int nextClassFlags) {
    context->nextClassFlags = static_cast<TypeDef::Flags>(nextClassFlags);
}

Location *CrackContext::getLocation(const char *name, int lineNumber) {
    return new Location(toker->getLocationMap().getLocation(name, lineNumber));
}

Location *CrackContext::getLocation() {
    return new Location(context->getLocation());
}

void CrackContext::continueIString() {
    toker->continueIString();
}

void CrackContext::_inject(CrackContext *inst, char *sourceName, int lineNumber, 
                           char *code
                           ) {
    istringstream iss(code);
    parser::Toker tempToker(iss, sourceName, lineNumber);
    list<parser::Token> tokens;
    parser::Token tok;
    while (!(tok = tempToker.getToken()).isEnd())
        tokens.push_front(tok);
    
    // transfer the tokens to the tokenizer
    while (!tokens.empty()) {
        inst->toker->putBack(tokens.front());
        tokens.pop_front();
    }
}

Token *CrackContext::_getToken(CrackContext *inst) {
    return new Token(inst->toker->getToken());
}

void CrackContext::_putBack(CrackContext *inst, Token *tok) {
    inst->toker->putBack(*tok->rep);
}

int CrackContext::_getScope(CrackContext *inst) {
    return inst->context->scope;
}

void CrackContext::_storeAnnotation(CrackContext *inst, const char *name, 
                                    AnnotationFunc func
                                    ) {
    inst->context->compileNS->addDef(new PrimFuncAnnotation(name, func));
}

compiler::Annotation *CrackContext::_getAnnotation(CrackContext *inst,
                                                   const char *name
                                                   ) {
    model::Annotation *ann = 
        model::AnnotationPtr::rcast(inst->context->compileNS->lookUp(name));
    return ann ? new Annotation(ann) : 0;
}

void CrackContext::_storeAnnotation(CrackContext *inst, const char *name, 
                                    AnnotationFunc func,
                                    void *userData
                                    ) {
    inst->context->compileNS->addDef(new PrimFuncAnnotation(name, func,
                                                            userData
                                                            )
                                    );
}

void *CrackContext::_getUserData(CrackContext *inst) {
    return inst->userData;
}

void CrackContext::_error(CrackContext *inst, const char *text) {
    inst->context->error(text, false);
}

void CrackContext::_error(CrackContext *inst, Token *tok, const char *text) {
    inst->context->error(tok->rep->getLocation(), text, false);
}

void CrackContext::_warn(CrackContext *inst, const char *text) {
    inst->context->warn(text);
}

void CrackContext::_warn(CrackContext *inst, Token *tok, const char *text) {
    inst->context->warn(tok->rep->getLocation(), text);
}

int CrackContext::_getParseState(CrackContext *inst) {
    return inst->parser->state;
}

void CrackContext::_pushErrorContext(CrackContext *inst, const char *text) {
    inst->context->pushErrorContext(text);
}

void CrackContext::_popErrorContext(CrackContext *inst) {
    inst->context->popErrorContext();
}

parser::ParserCallback *CrackContext::_addCallback(
    CrackContext *inst,     
    int event,
    CrackContext::AnnotationFunc func
) {
    parser::Parser::Event evt = static_cast<parser::Parser::Event>(event);
    Callback *callback = new Callback(evt, func);
    inst->parser->addCallback(evt, callback);
    return callback;
}

void CrackContext::_removeCallback(CrackContext *inst,
                                   parser::ParserCallback *callback
                                   ) {
    Callback *cb = dynamic_cast<Callback *>(callback);
    parser::Parser::Event event =
        static_cast<parser::Parser::Event>(cb->event);
    if (inst->parser->removeCallback(event, cb))
        delete callback;
    else
        inst->error("Attempted to remove a callback that wasn't registered.");
}

void CrackContext::_setNextFuncFlags(CrackContext *inst, int nextFuncFlags) {
    inst->context->nextFuncFlags = static_cast<FuncDef::Flags>(nextFuncFlags);
}

Location *CrackContext::_getLocation(CrackContext *inst, const char *name, 
                                     int lineNumber
                                     ) {
    return new Location(inst->toker->getLocationMap().getLocation(name, 
                                                                  lineNumber
                                                                  )
                        );
}

Location *CrackContext::_getLocation(CrackContext *inst) {
    return new Location(inst->context->getLocation());
}

void CrackContext::_continueIString(CrackContext *inst) {
    inst->toker->continueIString();
}
