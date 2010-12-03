// Copyright 2010 Google Inc.

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

vector<parser::ParserCallback *> callbacks;

void cleanUpCallbacks(CrackContext *ctx) {
    for (int i = 0; i < callbacks.size(); ++i)
        ctx->removeCallback(callbacks[i]);
    callbacks.clear();
}

void unexpectedElement(CrackContext *ctx) {
    ctx->error("Function expected after annotation");
}

void funcAnnCheck(CrackContext *ctx, const char *name) {
    if (ctx->getScope() != model::Context::composite ||
        ctx->getParseState() != parser::Parser::st_base
        )
        ctx->error(SPUG_FSTR(name << " annotation can not be used in this "
                                     "here (it must precede a function "
                                     "definition in a class body)"
                             ).c_str()
                   );
    
    callbacks.push_back(ctx->addCallback(parser::Parser::funcDef, 
                                         cleanUpCallbacks
                                         )
                        );
    callbacks.push_back(ctx->addCallback(parser::Parser::classDef,
                                         unexpectedElement
                                         )
                        );
    callbacks.push_back(ctx->addCallback(parser::Parser::exprBegin,
                                         unexpectedElement
                                         )
                        );
    callbacks.push_back(ctx->addCallback(parser::Parser::controlStmt,
                                         unexpectedElement
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

void init(Module *mod) {
    Func *f;
    Type *locationType = mod->addType("Location");
    locationType->addMethod(mod->getByteptrType(), "getName",
                            (void *)&Location::getName
                            );
    locationType->addMethod(mod->getIntType(), "getLineNumber",
                            (void *)&Location::getLineNumber
                            );
    locationType->addMethod(mod->getVoidType(), "oper bind",
                            (void *)&Location::bind
                            );
    locationType->addMethod(mod->getVoidType(), "oper release",
                            (void *)&Location::release
                            );
    locationType->finish();

    Type *tokenType = mod->addType("Token");
    f = tokenType->addStaticMethod(tokenType, "oper new",
                                   (void *)&Token::create
                                   );
    f->addArg(mod->getIntType(), "type");
    f->addArg(mod->getByteptrType(), "text");
    f->addArg(locationType, "loc");
    tokenType->addMethod(mod->getVoidType(), "oper bind",
                         (void *)&Token::bind
                         );
    tokenType->addMethod(mod->getVoidType(), "oper release",
                         (void *)&Token::release
                         );
    f = tokenType->addMethod(mod->getBoolType(), "hasText",
                             (void *)&Token::hasText
                             );
    f->addArg(mod->getByteptrType(), "text");
    
    tokenType->addMethod(mod->getByteptrType(), "getText",
                         (void *)&Token::getText
                         );
    tokenType->addMethod(mod->getIntType(), "getType",
                         (void *)&Token::getType
                         );
    tokenType->addMethod(locationType, "getLocation",
                         (void *)&Token::getLocation
                         );
    
    tokenType->addMethod(mod->getBoolType(), "isAnn", (void *)&Token::isAnn);
    tokenType->addMethod(mod->getBoolType(), "isBoolAnd", 
                         (void *)&Token::isBoolAnd
                         );
    tokenType->addMethod(mod->getBoolType(), "isBoolOr", 
                         (void *)&Token::isBoolOr
                         );
    tokenType->addMethod(mod->getBoolType(), "isIf", (void *)&Token::isIf);
    tokenType->addMethod(mod->getBoolType(), "isIn", (void *)&Token::isIn);
    tokenType->addMethod(mod->getBoolType(), "isImport",
                         (void *)&Token::isImport
                         );
    tokenType->addMethod(mod->getBoolType(), "isElse", (void *)&Token::isElse);
    tokenType->addMethod(mod->getBoolType(), "isOper", (void *)&Token::isOper);
    tokenType->addMethod(mod->getBoolType(), "isOn", (void *)&Token::isOn);
    tokenType->addMethod(mod->getBoolType(), "isWhile", (void *)&Token::isWhile);
    tokenType->addMethod(mod->getBoolType(), "isReturn", (void *)&Token::isReturn);
    tokenType->addMethod(mod->getBoolType(), "isBreak", (void *)&Token::isBreak);
    tokenType->addMethod(mod->getBoolType(), "isClass", (void *)&Token::isClass);
    tokenType->addMethod(mod->getBoolType(), "isContinue", (void *)&Token::isContinue);
    tokenType->addMethod(mod->getBoolType(), "isDollar", 
                         (void *)&Token::isDollar
                         );
    tokenType->addMethod(mod->getBoolType(), "isNull", (void *)&Token::isNull);
    tokenType->addMethod(mod->getBoolType(), "isIdent", (void *)&Token::isIdent);
    tokenType->addMethod(mod->getBoolType(), "isString", (void *)&Token::isString);
    tokenType->addMethod(mod->getBoolType(), "isIstrBegin", (void *)&Token::isIstrBegin);
    tokenType->addMethod(mod->getBoolType(), "isIstrEnd", (void *)&Token::isIstrEnd);
    tokenType->addMethod(mod->getBoolType(), "isSemi", (void *)&Token::isSemi);
    tokenType->addMethod(mod->getBoolType(), "isComma", (void *)&Token::isComma);
    tokenType->addMethod(mod->getBoolType(), "isColon", (void *)&Token::isColon);
    tokenType->addMethod(mod->getBoolType(), "isDecr", (void *)&Token::isDecr);
    tokenType->addMethod(mod->getBoolType(), "isDefine", (void *)&Token::isDefine);
    tokenType->addMethod(mod->getBoolType(), "isDot", (void *)&Token::isDot);
    tokenType->addMethod(mod->getBoolType(), "isIncr", (void *)&Token::isIncr);
    tokenType->addMethod(mod->getBoolType(), "isAssign", (void *)&Token::isAssign);
    tokenType->addMethod(mod->getBoolType(), "isLParen", (void *)&Token::isLParen);
    tokenType->addMethod(mod->getBoolType(), "isRParen", (void *)&Token::isRParen);
    tokenType->addMethod(mod->getBoolType(), "isLCurly", (void *)&Token::isLCurly);
    tokenType->addMethod(mod->getBoolType(), "isRCurly", (void *)&Token::isRCurly);
    tokenType->addMethod(mod->getBoolType(), "isLBracket", (void *)&Token::isLBracket);
    tokenType->addMethod(mod->getBoolType(), "isRBracket", (void *)&Token::isRBracket);
    tokenType->addMethod(mod->getBoolType(), "isInteger", (void *)&Token::isInteger);
    tokenType->addMethod(mod->getBoolType(), "isFloat", (void *)&Token::isFloat);
    tokenType->addMethod(mod->getBoolType(), "isOctal", (void *)&Token::isOctal);
    tokenType->addMethod(mod->getBoolType(), "isHex", (void *)&Token::isHex);
    tokenType->addMethod(mod->getBoolType(), "isBinary", (void *)&Token::isBinary);
    tokenType->addMethod(mod->getBoolType(), "isPlus", (void *)&Token::isPlus);
    tokenType->addMethod(mod->getBoolType(), "isQuest", (void *)&Token::isQuest);
    tokenType->addMethod(mod->getBoolType(), "isMinus", (void *)&Token::isMinus);
    tokenType->addMethod(mod->getBoolType(), "isAsterisk", (void *)&Token::isAsterisk);
    tokenType->addMethod(mod->getBoolType(), "isBang", (void *)&Token::isBang);
    tokenType->addMethod(mod->getBoolType(), "isSlash", (void *)&Token::isSlash);
    tokenType->addMethod(mod->getBoolType(), "isPercent", (void *)&Token::isPercent);
    tokenType->addMethod(mod->getBoolType(), "isNot", (void *)&Token::isNot);
    tokenType->addMethod(mod->getBoolType(), "isTilde", (void *)&Token::isTilde);
    tokenType->addMethod(mod->getBoolType(), "isGT", (void *)&Token::isGT);
    tokenType->addMethod(mod->getBoolType(), "isLT", (void *)&Token::isLT);
    tokenType->addMethod(mod->getBoolType(), "isEQ", (void *)&Token::isEQ);
    tokenType->addMethod(mod->getBoolType(), "isNE", (void *)&Token::isNE);
    tokenType->addMethod(mod->getBoolType(), "isGE", (void *)&Token::isGE);
    tokenType->addMethod(mod->getBoolType(), "isLE", (void *)&Token::isLE);
    tokenType->addMethod(mod->getBoolType(), "isEnd", (void *)&Token::isEnd);
    tokenType->addMethod(mod->getBoolType(), "isLogicAnd", (void *)&Token::isLogicAnd);
    tokenType->addMethod(mod->getBoolType(), "isLogicOr", (void *)&Token::isLogicOr);
    tokenType->addMethod(mod->getBoolType(), "isBinOp", (void *)&Token::isBinOp);
    tokenType->addMethod(mod->getBoolType(), "isAugAssign", (void *)&Token::isAugAssign);

    tokenType->finish();

    Type *opaqCallbackType = mod->addType("Callback");
    opaqCallbackType->finish();

    Type *annotationType = mod->addType("Annotation");
    annotationType->addMethod(mod->getVoidptrType(), "getUserData",
                              (void *)&Annotation::getUserData
                              );
    annotationType->addMethod(mod->getVoidptrType(), "getFunc",
                              (void *)&Annotation::getName
                              );
    annotationType->addMethod(mod->getVoidptrType(), "getName",
                              (void *)&Annotation::getName
                              );
    annotationType->finish();

    Type *cc = mod->addType("CrackContext");
    f = cc->addMethod(mod->getVoidType(), "inject",
                      (void *)&CrackContext::inject
                      );
    f->addArg(mod->getByteptrType(), "sourceName");
    f->addArg(mod->getIntType(), "lineNumber");
    f->addArg(mod->getByteptrType(), "code");
    
    cc->addMethod(tokenType, "getToken", (void *)&CrackContext::getToken);

    f = cc->addMethod(mod->getVoidType(), "putBack", 
                      (void *)&CrackContext::putBack
                      );
    f->addArg(tokenType, "tok");

    cc->addMethod(mod->getIntType(), "getScope",
                  (void *)&CrackContext::getScope
                  );
    
    cc->addMethod(mod->getVoidptrType(), "getUserData",
                  (void *)&CrackContext::getUserData
                  );

    typedef void (CrackContext::* G1)(const char *, void (*)(CrackContext *));
    G1 g1 = &CrackContext::storeAnnotation;
    f = cc->addMethod(mod->getVoidType(), "storeAnnotation", (void *)g1);
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getVoidptrType(), "func");

    typedef void (CrackContext::* G2)(const char *, void (*)(CrackContext *),
                                      void *
                                      );
    G2 g2 = &CrackContext::storeAnnotation;
    f = cc->addMethod(mod->getVoidType(), "storeAnnotation", (void *)g2);
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getVoidptrType(), "func");
    f->addArg(mod->getVoidptrType(), "userData");

    // error/warning functions

    // error(byteptr text)
    void (CrackContext::* g3)(const char *) = &CrackContext::error;
    f = cc->addMethod(mod->getVoidType(), "error", (void *)g3);
    f->addArg(mod->getByteptrType(), "text");
    
    // error(Token tok, byteptr text)
    void (CrackContext::* g4)(Token *, const char *) = &CrackContext::error;
    f = cc->addMethod(mod->getVoidType(), "error", (void *)g4);
    f->addArg(tokenType, "tok");
    f->addArg(mod->getByteptrType(), "text");

    // warn(byteptr text)
    g3 = &CrackContext::warn;
    f = cc->addMethod(mod->getVoidType(), "warn", (void *)g3);
    f->addArg(mod->getByteptrType(), "text");
                      
    g4 = &CrackContext::warn;
    f = cc->addMethod(mod->getVoidType(), "warn", (void *)g4);
    f->addArg(tokenType, "tok");
    f->addArg(mod->getByteptrType(), "text");
    
    f = cc->addMethod(mod->getVoidType(), "pushErrorContext",
                      (void *)&CrackContext::pushErrorContext
                      );
    f->addArg(mod->getByteptrType(), "text");
    
    cc->addMethod(mod->getVoidType(), "popErrorContext",
                  (void *)&CrackContext::popErrorContext
                  );

    cc->addMethod(mod->getIntType(), "getParseState", 
                  (void *)&CrackContext::getParseState
                  );

    f = cc->addMethod(opaqCallbackType, "addCallback",
                      (void *)&CrackContext::addCallback
                      );
    f->addArg(mod->getIntType(), "event");
    f->addArg(mod->getVoidptrType(), "callback");

    f = cc->addMethod(mod->getVoidType(), "removeCallback",
                      (void *)&CrackContext::removeCallback);
    f->addArg(opaqCallbackType, "callback");
    
    f = cc->addMethod(mod->getVoidType(), "setNextFuncFlags",
                      (void *)&CrackContext::setNextFuncFlags
                      );
    f->addArg(mod->getIntType(), "flags");

    typedef Location *(CrackContext::* L1)();
    typedef Location *(CrackContext::* L2)(const char *, int);
    f = cc->addMethod(locationType, "getLocation",
                      (void *)static_cast<L2>(&CrackContext::getLocation)
                      );
    f->addArg(mod->getByteptrType(), "name");
    f->addArg(mod->getIntType(), "lineNumber");
    cc->addMethod(locationType, "getLocation",
                  (void *)static_cast<L1>(&CrackContext::getLocation)
                  );

    f = cc->addMethod(annotationType, "getAnnotation",
                      (void *)&CrackContext::getAnnotation
                      );
    f->addArg(mod->getByteptrType(), "name");

    cc->finish();
    
    // our annotations
    f = mod->addFunc(mod->getVoidType(), "static", (void *)staticAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "final", (void *)finalAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getByteptrType(), "FILE", (void *)fileAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getByteptrType(), "LINE", (void *)lineAnn);
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
    mod->addConstant(mod->getIntType(), "PCB_FUNC_DEF", 
                     parser::Parser::funcDef
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
                    
}

} // namespace compiler
