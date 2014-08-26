// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "init.h"

#include "spug/StringFmt.h"
#include "model/FuncDef.h"
#include "parser/Parser.h"
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "Annotation.h"
#include "CrackContext.h"
#include "Token.h"
#include "Location.h"

using namespace std;
using namespace crack::ext;

namespace compiler {

struct CallbackManager {
    vector<parser::ParserCallback *> callbacks;
    const char *errorText;

    CallbackManager(const char *errorText) : errorText(errorText) {}

    void cleanUpCallbacks(CrackContext *ctx) {
        assert(callbacks.size() > 0);
        for (int i = 0; i < callbacks.size(); ++i)
            ctx->removeCallback(callbacks[i]);
        callbacks.clear();
    }

    void unexpectedElement(CrackContext *ctx) {
        ctx->error(errorText);
        cleanUpCallbacks(ctx);
        ctx->setNextFuncFlags(model::FuncDef::noFlags);
        ctx->setNextClassFlags(model::TypeDef::noFlags);
        delete this;
    }

    void cleanUpAfterClass(CrackContext *ctx) {
        cleanUpCallbacks(ctx);
        ctx->setNextFuncFlags(model::FuncDef::noFlags);
        delete this;
    }

    void cleanUpAfterFunc(CrackContext *ctx) {
        cleanUpCallbacks(ctx);
        ctx->setNextClassFlags(model::TypeDef::noFlags);
        delete this;
    }

    void add(parser::ParserCallback *cb) {
        callbacks.push_back(cb);
    }
};

typedef CrackContext::AnnotationFuncWrapper<CallbackManager> CallbackBatch;

void funcAnnCheck(CrackContext *ctx, const char *name) {
    parser::Parser::State parseState =
        static_cast<parser::Parser::State>(ctx->getParseState());
    if (ctx->getScope() != model::Context::composite ||
        (parseState != parser::Parser::st_base &&
         parseState != parser::Parser::st_optElse
         )
        )
        ctx->error(SPUG_FSTR(name << " annotation can not be used  here (it "
                                     "must precede a function "
                                     "definition in a class body)"
                             ).c_str()
                   );

    CallbackManager *cbm =
        new CallbackManager("Function expected after annotation");
    cbm->add(ctx->addCallback(parser::Parser::funcDef,
                              new CallbackBatch(
                                  &CallbackManager::cleanUpAfterFunc,
                                  cbm
                               )
                              )
             );
    cbm->add(ctx->addCallback(parser::Parser::classDef,
                              new CallbackBatch(
                                  &CallbackManager::unexpectedElement,
                                  cbm
                               )
                              )
             );
    cbm->add(ctx->addCallback(parser::Parser::exprBegin,
                              new CallbackBatch(
                                  &CallbackManager::unexpectedElement,
                                  cbm
                               )
                              )
             );
    cbm->add(ctx->addCallback(parser::Parser::controlStmt,
                              new CallbackBatch(
                                  &CallbackManager::unexpectedElement,
                                  cbm
                               )
                              )
             );
}

void staticAnn(CrackContext *ctx) {
    funcAnnCheck(ctx, "static");
    ctx->setNextFuncFlags(model::FuncDef::explicitFlags);
}

void finalAnn(CrackContext *ctx) {
    funcAnnCheck(ctx, "final");
    ctx->setNextFuncFlags(model::FuncDef::explicitFlags |
                          model::FuncDef::method
                          );
}

void abstractAnn(CrackContext *ctx) {

    // @abstract is not strictly a function annotation, it may precede a class
    // definition.
    parser::Parser::State parseState =
        static_cast<parser::Parser::State>(ctx->getParseState());
    if (parseState != parser::Parser::st_base &&
        parseState != parser::Parser::st_optElse
        )
        ctx->error("abstract annotation can not be used here (it must precede "
                    "a function or class definition)"
                   );

    CallbackManager *cbm =
        new CallbackManager("Function or class expected after @abstract "
                             "annotation"
                            );
    cbm->add(
        ctx->addCallback(parser::Parser::funcDef,
                         new CallbackBatch(
                             &CallbackManager::cleanUpAfterFunc,
                             cbm
                          )
                         )
    );
    cbm->add(
        ctx->addCallback(parser::Parser::classDef,
                         new CallbackBatch(
                             &CallbackManager::cleanUpAfterClass,
                            cbm
                          )
                         )
    );
    cbm->add(
        ctx->addCallback(parser::Parser::exprBegin,
                         new CallbackBatch(
                             &CallbackManager::unexpectedElement,
                             cbm
                          )
                         )
    );
    cbm->add(
        ctx->addCallback(parser::Parser::controlStmt,
                         new CallbackBatch(
                             &CallbackManager::unexpectedElement,
                             cbm
                          )
                         )
    );

    ctx->setNextFuncFlags(model::FuncDef::explicitFlags |
                          model::FuncDef::method |
                          model::FuncDef::virtualized |
                          model::FuncDef::abstract
                          );
    ctx->setNextClassFlags(model::TypeDef::explicitFlags |
                           model::TypeDef::abstractClass
                           );
}

void fileAnn(CrackContext *ctx) {
    Location *loc = ctx->getLocation();
    Token *newTok = new Token(parser::Token::string, loc->getName(), loc);
    ctx->putBack(newTok);
    loc->release();
    newTok->release();
}

void lineAnn(CrackContext *ctx) {
    Location *loc = ctx->getLocation();
    Token *newTok = new Token(parser::Token::integer,
                              SPUG_FSTR(loc->getLineNumber()).c_str(),
                              loc
                              );
    ctx->putBack(newTok);
    loc->release();
    newTok->release();
}

void encodingAnn(CrackContext *ctx) {
    Token *tok = ctx->getToken();
    bool isString = tok->isString();
    tok->release();
    if (!isString)
        ctx->error("String expected after 'encoding' annotation");
}

void export_symbolsAnn(CrackContext *ctx) {

    // get the module namespace
    model::Context *realCtx = ctx->getContext();
    model::ModuleDefPtr mod =
        model::ModuleDefPtr::rcast(realCtx->getModuleContext()->ns);

    while (true) {
        Token *tok = ctx->getToken();
        if (!tok->isIdent())
            ctx->error("Identifier expected.");

        // add the symbol to the module's exports
        mod->exports[tok->getText()] = true;

        // check for a comma or semicolon
        tok = ctx->getToken();
        if (tok->isSemi())
            break;
        else if (!tok->isComma())
            ctx->error("Comma or semicolon expected in export_symbols.");
    }
}

extern "C" void crack_compiler_rinit() {}

void init(Module *mod) {
    Func *f;
    Type *locationType = mod->addType("Location", sizeof(Location));
    locationType->addMethod(mod->getByteptrType(), "getName",
                            (void *)Location::_getName
                            );
    locationType->addMethod(mod->getIntType(), "getLineNumber",
                            (void *)Location::_getLineNumber
                            );
    locationType->addMethod(mod->getVoidType(), "oper bind",
                            (void *)Location::_bind
                            );
    locationType->addMethod(mod->getVoidType(), "oper release",
                            (void *)Location::_release
                            );
    locationType->finish();

    Type *tokenType = mod->addType("Token", sizeof(Token));
    f = tokenType->addStaticMethod(tokenType, "oper new",
                                   (void *)&Token::create
                                   );
    f->addArg(mod->getIntType(), "type");
    f->addArg(mod->getByteptrType(), "text");
    f->addArg(locationType, "loc");
    tokenType->addMethod(mod->getVoidType(), "oper bind",
                         (void *)Token::_bind
                         );
    tokenType->addMethod(mod->getVoidType(), "oper release",
                         (void *)Token::_release
                         );
    f = tokenType->addMethod(mod->getBoolType(), "hasText",
                             (void *)Token::_hasText
                             );
    f->addArg(mod->getByteptrType(), "text");

    tokenType->addMethod(mod->getByteptrType(), "getText",
                         (void *)Token::_getText
                         );
    tokenType->addMethod(mod->getIntType(), "getType",
                         (void *)Token::_getType
                         );
    tokenType->addMethod(locationType, "getLocation",
                         (void *)Token::_getLocation
                         );

    tokenType->addMethod(mod->getBoolType(), "isAlias",
                         (void *)Token::_isAlias
                         );
    tokenType->addMethod(mod->getBoolType(), "isAnn", (void *)Token::_isAnn);
    tokenType->addMethod(mod->getBoolType(), "isBoolAnd",
                         (void *)Token::_isBoolAnd
                         );
    tokenType->addMethod(mod->getBoolType(), "isBoolOr",
                         (void *)Token::_isBoolOr
                         );
    tokenType->addMethod(mod->getBoolType(), "isCase",
                         (void *)Token::_isCase
                         );
    tokenType->addMethod(mod->getBoolType(), "isIf", (void *)Token::_isIf);
    tokenType->addMethod(mod->getBoolType(), "isIn", (void *)Token::_isIn);
    tokenType->addMethod(mod->getBoolType(), "isImport",
                         (void *)Token::_isImport
                         );
    tokenType->addMethod(mod->getBoolType(), "isElse", (void *)Token::_isElse);
    tokenType->addMethod(mod->getBoolType(), "isOper", (void *)Token::_isOper);
    tokenType->addMethod(mod->getBoolType(), "isOn", (void *)Token::_isOn);
    tokenType->addMethod(mod->getBoolType(), "isWhile",
                         (void *)Token::_isWhile);
    tokenType->addMethod(mod->getBoolType(), "isReturn",
                         (void *)Token::_isReturn);
    tokenType->addMethod(mod->getBoolType(), "isSwitch",
                         (void *)Token::_isSwitch
                         );
    tokenType->addMethod(mod->getBoolType(), "isBreak",
                         (void *)Token::_isBreak
                         );
    tokenType->addMethod(mod->getBoolType(), "isClass",
                         (void *)Token::_isClass
                         );
    tokenType->addMethod(mod->getBoolType(), "isContinue",
                         (void *)Token::_isContinue
                         );
    tokenType->addMethod(mod->getBoolType(), "isDollar",
                         (void *)Token::_isDollar
                         );
    tokenType->addMethod(mod->getBoolType(), "isNull", (void *)Token::_isNull);
    tokenType->addMethod(mod->getBoolType(), "isIdent", (void *)Token::_isIdent);
    tokenType->addMethod(mod->getBoolType(), "isString", (void *)Token::_isString);
    tokenType->addMethod(mod->getBoolType(), "isIstrBegin", (void *)Token::_isIstrBegin);
    tokenType->addMethod(mod->getBoolType(), "isIstrEnd", (void *)Token::_isIstrEnd);
    tokenType->addMethod(mod->getBoolType(), "isSemi", (void *)Token::_isSemi);
    tokenType->addMethod(mod->getBoolType(), "isComma", (void *)Token::_isComma);
    tokenType->addMethod(mod->getBoolType(), "isColon", (void *)Token::_isColon);
    tokenType->addMethod(mod->getBoolType(), "isConst", (void *)Token::_isConst);
    tokenType->addMethod(mod->getBoolType(), "isDecr", (void *)Token::_isDecr);
    tokenType->addMethod(mod->getBoolType(), "isDefine", (void *)Token::_isDefine);
    tokenType->addMethod(mod->getBoolType(), "isDot", (void *)Token::_isDot);
    tokenType->addMethod(mod->getBoolType(), "isIncr", (void *)Token::_isIncr);
    tokenType->addMethod(mod->getBoolType(), "isAssign", (void *)Token::_isAssign);
    tokenType->addMethod(mod->getBoolType(), "isLambda", (void *)Token::_isLambda);
    tokenType->addMethod(mod->getBoolType(), "isLParen", (void *)Token::_isLParen);
    tokenType->addMethod(mod->getBoolType(), "isModule", (void *)Token::_isModule);
    tokenType->addMethod(mod->getBoolType(), "isRParen", (void *)Token::_isRParen);
    tokenType->addMethod(mod->getBoolType(), "isLCurly", (void *)Token::_isLCurly);
    tokenType->addMethod(mod->getBoolType(), "isRCurly", (void *)Token::_isRCurly);
    tokenType->addMethod(mod->getBoolType(), "isLBracket", (void *)Token::_isLBracket);
    tokenType->addMethod(mod->getBoolType(), "isRBracket", (void *)Token::_isRBracket);
    tokenType->addMethod(mod->getBoolType(), "isInteger", (void *)Token::_isInteger);
    tokenType->addMethod(mod->getBoolType(), "isFloat", (void *)Token::_isFloat);
    tokenType->addMethod(mod->getBoolType(), "isOctal", (void *)Token::_isOctal);
    tokenType->addMethod(mod->getBoolType(), "isHex", (void *)Token::_isHex);
    tokenType->addMethod(mod->getBoolType(), "isBinary", (void *)Token::_isBinary);
    tokenType->addMethod(mod->getBoolType(), "isPlus", (void *)Token::_isPlus);
    tokenType->addMethod(mod->getBoolType(), "isQuest", (void *)Token::_isQuest);
    tokenType->addMethod(mod->getBoolType(), "isMinus", (void *)Token::_isMinus);
    tokenType->addMethod(mod->getBoolType(), "isAsterisk", (void *)Token::_isAsterisk);
    tokenType->addMethod(mod->getBoolType(), "isBang", (void *)Token::_isBang);
    tokenType->addMethod(mod->getBoolType(), "isSlash", (void *)Token::_isSlash);
    tokenType->addMethod(mod->getBoolType(), "isPercent", (void *)Token::_isPercent);
    tokenType->addMethod(mod->getBoolType(), "isNot", (void *)Token::_isNot);
    tokenType->addMethod(mod->getBoolType(), "isTilde", (void *)Token::_isTilde);
    tokenType->addMethod(mod->getBoolType(), "isGT", (void *)Token::_isGT);
    tokenType->addMethod(mod->getBoolType(), "isLT", (void *)Token::_isLT);
    tokenType->addMethod(mod->getBoolType(), "isEQ", (void *)Token::_isEQ);
    tokenType->addMethod(mod->getBoolType(), "isNE", (void *)Token::_isNE);
    tokenType->addMethod(mod->getBoolType(), "isGE", (void *)Token::_isGE);
    tokenType->addMethod(mod->getBoolType(), "isLE", (void *)Token::_isLE);
    tokenType->addMethod(mod->getBoolType(), "isEnd", (void *)Token::_isEnd);
    tokenType->addMethod(mod->getBoolType(), "isLogicAnd", (void *)Token::_isLogicAnd);
    tokenType->addMethod(mod->getBoolType(), "isLogicOr", (void *)Token::_isLogicOr);
    tokenType->addMethod(mod->getBoolType(), "isScoping", (void *)Token::_isDefine);
    tokenType->addMethod(mod->getBoolType(), "isBinOp", (void *)Token::_isBinOp);
    tokenType->addMethod(mod->getBoolType(), "isAugAssign", (void *)Token::_isAugAssign);

    tokenType->finish();

    Type *opaqCallbackType = mod->addType("Callback", 0);
    opaqCallbackType->finish();

    Type *annotationType = mod->addType("Annotation", sizeof(Annotation));
    annotationType->addMethod(mod->getVoidptrType(), "getUserData",
                              (void *)Annotation::_getUserData
                              );
    annotationType->addMethod(mod->getVoidptrType(), "getFunc",
                              (void *)Annotation::_getName
                              );
    annotationType->addMethod(mod->getVoidptrType(), "getName",
                              (void *)Annotation::_getName
                              );
    annotationType->finish();

    Type *cc = mod->addType("CrackContext", sizeof(CrackContext));
    f = cc->addMethod(mod->getVoidType(), "inject",
                      (void *)CrackContext::_inject
                      );
    f->addArg(mod->getByteptrType(), "sourceName");
    f->addArg(mod->getIntType(), "lineNumber");
    f->addArg(mod->getByteptrType(), "code");

    cc->addMethod(tokenType, "getToken", (void *)CrackContext::_getToken);

    f = cc->addMethod(mod->getVoidType(), "putBack",
                      (void *)CrackContext::_putBack
                      );
    f->addArg(tokenType, "tok");

    cc->addMethod(mod->getIntType(), "getScope",
                  (void *)CrackContext::_getScope
                  );

    cc->addMethod(mod->getVoidptrType(), "getUserData",
                  (void *)CrackContext::_getUserData
                  );

    typedef void (*G1)(CrackContext *, const char *, void (*)(CrackContext *));
    G1 g1 = CrackContext::_storeAnnotation;
    f = cc->addMethod(mod->getVoidType(), "storeAnnotation", (void *)g1);
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getVoidptrType(), "func");

    typedef void (*G2)(CrackContext *, const char *, void (*)(CrackContext *),
                       void *
                       );
    G2 g2 = CrackContext::_storeAnnotation;
    f = cc->addMethod(mod->getVoidType(), "storeAnnotation", (void *)g2);
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getVoidptrType(), "func");
    f->addArg(mod->getVoidptrType(), "userData");

    // error/warning functions

    // error(byteptr text)
    void (*g3)(CrackContext *, const char *) = CrackContext::_error;
    f = cc->addMethod(mod->getVoidType(), "error", (void *)g3);
    f->addArg(mod->getByteptrType(), "text");

    // error(Token tok, byteptr text)
    void (*g4)(CrackContext *, Token *, const char *) = CrackContext::_error;
    f = cc->addMethod(mod->getVoidType(), "error", (void *)g4);
    f->addArg(tokenType, "tok");
    f->addArg(mod->getByteptrType(), "text");

    // warn(byteptr text)
    g3 = CrackContext::_warn;
    f = cc->addMethod(mod->getVoidType(), "warn", (void *)g3);
    f->addArg(mod->getByteptrType(), "text");

    g4 = CrackContext::_warn;
    f = cc->addMethod(mod->getVoidType(), "warn", (void *)g4);
    f->addArg(tokenType, "tok");
    f->addArg(mod->getByteptrType(), "text");

    f = cc->addMethod(mod->getVoidType(), "pushErrorContext",
                      (void *)CrackContext::_pushErrorContext
                      );
    f->addArg(mod->getByteptrType(), "text");

    cc->addMethod(mod->getVoidType(), "popErrorContext",
                  (void *)CrackContext::_popErrorContext
                  );

    cc->addMethod(mod->getIntType(), "getParseState",
                  (void *)CrackContext::_getParseState
                  );

    f = cc->addMethod(opaqCallbackType, "addCallback",
                      (void *)CrackContext::_addCallback
                      );
    f->addArg(mod->getIntType(), "event");
    f->addArg(mod->getVoidptrType(), "callback");

    f = cc->addMethod(mod->getVoidType(), "removeCallback",
                      (void *)CrackContext::_removeCallback);
    f->addArg(opaqCallbackType, "callback");

    f = cc->addMethod(mod->getVoidType(), "setNextFuncFlags",
                      (void *)CrackContext::_setNextFuncFlags
                      );
    f->addArg(mod->getIntType(), "flags");

    f = cc->addMethod(mod->getUintType(), "getCurrentVTableOffset",
                      (void *)CrackContext::_getCurrentVTableOffset
                      );

    typedef Location *(*L1)(CrackContext *);
    typedef Location *(*L2)(CrackContext *, const char *, int);
    f = cc->addMethod(locationType, "getLocation",
                      (void *)static_cast<L2>(CrackContext::_getLocation)
                      );
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getIntType(), "lineNumber");
    cc->addMethod(locationType, "getLocation",
                  (void *)static_cast<L1>(CrackContext::_getLocation)
                  );

    f = cc->addMethod(annotationType, "getAnnotation",
                      (void *)CrackContext::_getAnnotation
                      );
    f->addArg(mod->getByteptrType(), "name");

    cc->addMethod(mod->getVoidType(), "continueIString",
                  (void *)CrackContext::_continueIString
                  );

    cc->finish();

    // our annotations
    f = mod->addFunc(mod->getVoidType(), "static", (void *)staticAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "final", (void *)finalAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "abstract", (void *)abstractAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getByteptrType(), "FILE", (void *)fileAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getByteptrType(), "LINE", (void *)lineAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "encoding", (void *)encodingAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "export_symbols",
                     (void *)export_symbolsAnn);
    f->addArg(cc, "ctx");

    // constants
    mod->addConstant(mod->getIntType(), "TOK_", 0);
    mod->addConstant(mod->getIntType(), "TOK_ANN", parser::Token::ann);
    mod->addConstant(mod->getIntType(), "TOK_BITAND", parser::Token::bitAnd);
    mod->addConstant(mod->getIntType(), "TOK_BITLSH", parser::Token::bitLSh);
    mod->addConstant(mod->getIntType(), "TOK_BITOR", parser::Token::bitOr);
    mod->addConstant(mod->getIntType(), "TOK_BITRSH", parser::Token::bitRSh);
    mod->addConstant(mod->getIntType(), "TOK_BITXOR", parser::Token::bitXor);
    mod->addConstant(mod->getIntType(), "TOK_BREAKKW",
                     parser::Token::breakKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_CLASSKW",
                     parser::Token::classKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_CONTINUEKW",
                     parser::Token::continueKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_DOLLAR",
                     parser::Token::dollar
                     );
    mod->addConstant(mod->getIntType(), "TOK_FORKW", parser::Token::forKw);
    mod->addConstant(mod->getIntType(), "TOK_ELSEKW", parser::Token::elseKw);
    mod->addConstant(mod->getIntType(), "TOK_IFKW", parser::Token::ifKw);
    mod->addConstant(mod->getIntType(), "TOK_IMPORTKW",
                     parser::Token::importKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_INKW", parser::Token::inKw);
    mod->addConstant(mod->getIntType(), "TOK_ISKW", parser::Token::isKw);
    mod->addConstant(mod->getIntType(), "TOK_NULLKW", parser::Token::nullKw);
    mod->addConstant(mod->getIntType(), "TOK_ONKW", parser::Token::onKw);
    mod->addConstant(mod->getIntType(), "TOK_OPERKW", parser::Token::operKw);
    mod->addConstant(mod->getIntType(), "TOK_RETURNKW",
                     parser::Token::returnKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_WHILEKW",
                     parser::Token::whileKw
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGN", parser::Token::assign);
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNAND",
                     parser::Token::assignAnd
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNASTERISK",
                     parser::Token::assignAsterisk
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNLSH",
                     parser::Token::assignLSh
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNOR",
                     parser::Token::assignOr
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNRSH",
                     parser::Token::assignRSh
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNXOR",
                     parser::Token::assignXor
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNMINUS",
                     parser::Token::assignMinus
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNPERCENT",
                     parser::Token::assignPercent
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNPLUS",
                     parser::Token::assignPlus
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASSIGNSLASH",
                     parser::Token::assignSlash
                     );
    mod->addConstant(mod->getIntType(), "TOK_ASTERISK",
                     parser::Token::asterisk
                     );
    mod->addConstant(mod->getIntType(), "TOK_BANG", parser::Token::bang);
    mod->addConstant(mod->getIntType(), "TOK_COLON", parser::Token::colon);
    mod->addConstant(mod->getIntType(), "TOK_COMMA", parser::Token::comma);
    mod->addConstant(mod->getIntType(), "TOK_DECR", parser::Token::decr);
    mod->addConstant(mod->getIntType(), "TOK_DEFINE", parser::Token::define);
    mod->addConstant(mod->getIntType(), "TOK_DOT", parser::Token::dot);
    mod->addConstant(mod->getIntType(), "TOK_END", parser::Token::end);
    mod->addConstant(mod->getIntType(), "TOK_EQ", parser::Token::eq);
    mod->addConstant(mod->getIntType(), "TOK_GE", parser::Token::ge);
    mod->addConstant(mod->getIntType(), "TOK_GT", parser::Token::gt);
    mod->addConstant(mod->getIntType(), "TOK_IDENT", parser::Token::ident);
    mod->addConstant(mod->getIntType(), "TOK_INCR", parser::Token::incr);
    mod->addConstant(mod->getIntType(), "TOK_INTEGER",
                     parser::Token::integer
                     );
    mod->addConstant(mod->getIntType(), "TOK_LBRACKET",
                     parser::Token::lbracket
                     );
    mod->addConstant(mod->getIntType(), "TOK_LCURLY", parser::Token::lcurly);
    mod->addConstant(mod->getIntType(), "TOK_LE", parser::Token::le);
    mod->addConstant(mod->getIntType(), "TOK_LPAREN", parser::Token::lparen);
    mod->addConstant(mod->getIntType(), "TOK_LT", parser::Token::lt);
    mod->addConstant(mod->getIntType(), "TOK_MINUS", parser::Token::minus);
    mod->addConstant(mod->getIntType(), "TOK_NE", parser::Token::ne);
    mod->addConstant(mod->getIntType(), "TOK_PERCENT",
                     parser::Token::percent
                     );
    mod->addConstant(mod->getIntType(), "TOK_PLUS", parser::Token::plus);
    mod->addConstant(mod->getIntType(), "TOK_QUEST", parser::Token::quest);
    mod->addConstant(mod->getIntType(), "TOK_RBRACKET",
                     parser::Token::rbracket);
    mod->addConstant(mod->getIntType(), "TOK_RCURLY", parser::Token::rcurly);
    mod->addConstant(mod->getIntType(), "TOK_RPAREN", parser::Token::rparen);
    mod->addConstant(mod->getIntType(), "TOK_SEMI", parser::Token::semi);
    mod->addConstant(mod->getIntType(), "TOK_SLASH", parser::Token::slash);
    mod->addConstant(mod->getIntType(), "TOK_STRING", parser::Token::string);
    mod->addConstant(mod->getIntType(), "TOK_TILDE", parser::Token::tilde);
    mod->addConstant(mod->getIntType(), "TOK_ISTRBEGIN",
                     parser::Token::istrBegin
                     );
    mod->addConstant(mod->getIntType(), "TOK_ISTREND", parser::Token::istrEnd);
    mod->addConstant(mod->getIntType(), "TOK_LOGICAND",
                     parser::Token::logicAnd
                     );
    mod->addConstant(mod->getIntType(), "TOK_LOGICOR",
                     parser::Token::logicOr
                     );
    mod->addConstant(mod->getIntType(), "TOK_FLOATLIT",
                     parser::Token::floatLit
                     );
    mod->addConstant(mod->getIntType(), "TOK_OCTALLIT",
                     parser::Token::octalLit
                     );
    mod->addConstant(mod->getIntType(), "TOK_HEXLIT", parser::Token::hexLit);
    mod->addConstant(mod->getIntType(), "TOK_BINLIT", parser::Token::binLit);
    mod->addConstant(mod->getIntType(), "TOK_POPERRCTX",
                     parser::Token::popErrCtx
                     );

    mod->addConstant(mod->getIntType(), "SCOPE_MODULE", 0);
    mod->addConstant(mod->getIntType(), "SCOPE_FUNCTION", 2);
    mod->addConstant(mod->getIntType(), "SCOPE_CLASS", 3);
    mod->addConstant(mod->getIntType(), "STATE_BASE", parser::Parser::st_base);
    mod->addConstant(mod->getIntType(), "STATE_OPT_ELSE",
                     parser::Parser::st_optElse
                     );
    mod->addConstant(mod->getIntType(), "PCB_FUNC_DEF",
                     parser::Parser::funcDef
                     );
    mod->addConstant(mod->getIntType(), "PCB_FUNC_FORWARD",
                     parser::Parser::funcForward
                     );
    mod->addConstant(mod->getIntType(), "PCB_FUNC_ENTER",
                     parser::Parser::funcEnter
                     );
    mod->addConstant(mod->getIntType(), "PCB_FUNC_LEAVE",
                     parser::Parser::funcLeave
                     );
    mod->addConstant(mod->getIntType(), "PCB_CLASS_DEF",
                     parser::Parser::classDef
                     );
    mod->addConstant(mod->getIntType(), "PCB_CLASS_ENTER",
                     parser::Parser::classEnter
                     );
    mod->addConstant(mod->getIntType(), "PCB_CLASS_LEAVE",
                     parser::Parser::classLeave
                     );
    mod->addConstant(mod->getIntType(), "PCB_VAR_DEF",
                     parser::Parser::variableDef
                     );
    mod->addConstant(mod->getIntType(), "PCB_EXPR_BEGIN",
                     parser::Parser::exprBegin
                     );
    mod->addConstant(mod->getIntType(), "PCB_CONTROL_STMT",
                     parser::Parser::controlStmt
                     );

    mod->addConstant(mod->getIntType(), "FUNCFLAG_STATIC",
                     model::FuncDef::explicitFlags
                     );
    mod->addConstant(mod->getIntType(), "FUNCFLAG_FINAL",
                     model::FuncDef::explicitFlags | model::FuncDef::method
                     );
    mod->addConstant(mod->getIntType(), "FUNCFLAG_ABSTRACT",
                     model::FuncDef::explicitFlags | model::FuncDef::abstract
                     );
    mod->addConstant(mod->getIntType(), "CLASSFLAG_ABSTRACT",
                     model::FuncDef::explicitFlags |
                      model::TypeDef::abstractClass
                     );

}

} // namespace compiler
