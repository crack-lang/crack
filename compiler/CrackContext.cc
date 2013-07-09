// Copyright 2010-2012 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
    struct Callback : parser::ParserCallback {
        parser::Parser::Event event;
        
        Callback(parser::Parser::Event event) : event(event) {}
    };

    // Implements parser callback to wrap raw functions.
    struct FunctionCallback : public Callback {
        CrackContext::AnnotationFunc func;

        FunctionCallback(parser::Parser::Event event,
                 CrackContext::AnnotationFunc func
                 ) :
            Callback(event),
            func(func) {
        }

        virtual void run(parser::Parser *parser, parser::Toker *toker, 
                         model::Context *context
                         ) {
            CrackContext ctx(parser, toker, context);
            func(&ctx);
        }
    };

    // Implements parser callback to wrap functor callbacks.
    // This only exists because the parser namespace is private, otherwise we 
    // could just derive out callbacks from the parser's.
    struct FunctorCallback : public Callback {
        CrackContext::AnnotationFunctor *functor;

        FunctorCallback(parser::Parser::Event event,
                        CrackContext::AnnotationFunctor *functor
                        ) :
            Callback(event),
            functor(functor) {
        }

        virtual void run(parser::Parser *parser, parser::Toker *toker, 
                         model::Context *context
                         ) {
            CrackContext ctx(parser, toker, context);
            functor->run(&ctx);
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
    
    // 0 - not in an istring
    // 1 - in an istring
    // 2 - in an interpolation sequence
    // >2 - in an interpolation sequence nested in n-2 parenthesis
    int istrMode = 0;
    while (!(tok = tempToker.getToken()).isEnd()) {
        if (istrMode) {
            if (tok.isLParen()){
                ++istrMode;
            } else if (tok.isRParen() && --istrMode == 1) {
                tempToker.continueIString();
            } else if (istrMode == 1 && tok.isIdent()) {
                --istrMode;
                tempToker.continueIString();
            } else if (tok.isIstrEnd()) {
                assert(istrMode == 1);
                --istrMode;
            }
        } else if (tok.isIstrBegin()) {
            ++istrMode;
        }
                    
        tokens.push_front(tok);
    }
    
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
    FunctionCallback *callback = new FunctionCallback(evt, func);
    parser->addCallback(evt, callback);
    return callback;
}

parser::ParserCallback *CrackContext::addCallback(
    int event,
    CrackContext::AnnotationFunctor *functor
) {
    parser::Parser::Event evt = static_cast<parser::Parser::Event>(event);
    FunctorCallback *callback = new FunctorCallback(evt, functor);
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

unsigned int CrackContext::getCurrentVTableOffset() const {
    return context->vtableOffset;
}

Location *CrackContext::getLocation(const char *name, int lineNumber) {
    return new Location(new parser::LocationImpl(name, lineNumber));
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
    inst->inject(sourceName, lineNumber, code);
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
    FunctionCallback *callback = new FunctionCallback(evt, func);
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

unsigned int CrackContext::_getCurrentVTableOffset(CrackContext *inst) {
    return inst->context->vtableOffset;
}

Location *CrackContext::_getLocation(CrackContext *inst, const char *name,
                                     int lineNumber
                                     ) {
    return new Location(new parser::LocationImpl(name, lineNumber));
}

Location *CrackContext::_getLocation(CrackContext *inst) {
    return new Location(inst->context->getLocation());
}

void CrackContext::_continueIString(CrackContext *inst) {
    inst->toker->continueIString();
}
