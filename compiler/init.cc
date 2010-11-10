// Copyright 2010 Google Inc.

#include "init.h"

#include "spug/StringFmt.h"
#include "model/FuncDef.h"
#include "parser/Parser.h"
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "CrackContext.h"
#include "Token.h"

using namespace crack::ext;

namespace compiler {

void funcAnnCheck(CrackContext *ctx, const char *name) {
    if (ctx->getScope() != model::Context::composite ||
        ctx->getParseState() != parser::Parser::st_base
        )
        ctx->error(SPUG_FSTR(name << " annotation can not be used in this "
                                     "here (it must precede a function "
                                     "definition in a class body)"
                             ).c_str()
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

void init(Module *mod) {
    Type *tokenType = mod->addType("Token");
    tokenType->addMethod(mod->getVoidType(), "oper bind",
                         (void *)&Token::bind
                         );
    tokenType->addMethod(mod->getVoidType(), "oper release",
                         (void *)&Token::release
                         );
    Func *f = tokenType->addMethod(mod->getBoolType(), "hasText",
                                   (void *)&Token::hasText
                                   );
    f->addArg(mod->getByteptrType(), "text");
    
    tokenType->addMethod(mod->getByteptrType(), "getText",
                         (void *)&Token::getText
                         );
    
    tokenType->addMethod(mod->getBoolType(), "isAnn", (void *)&Token::isAnn);
    tokenType->addMethod(mod->getBoolType(), "isBoolAnd", 
                         (void *)&Token::isBoolAnd
                         );
    tokenType->addMethod(mod->getBoolType(), "isBoolOr", 
                         (void *)&Token::isBoolOr
                         );
    tokenType->addMethod(mod->getBoolType(), "isIf", (void *)&Token::isIf);
    tokenType->addMethod(mod->getBoolType(), "isImport",
                         (void *)&Token::isImport
                         );
    tokenType->addMethod(mod->getBoolType(), "isElse", (void *)&Token::isElse);
    tokenType->addMethod(mod->getBoolType(), "isOper", (void *)&Token::isOper);
    tokenType->addMethod(mod->getBoolType(), "isWhile", (void *)&Token::isWhile);
    tokenType->addMethod(mod->getBoolType(), "isReturn", (void *)&Token::isReturn);
    tokenType->addMethod(mod->getBoolType(), "isBreak", (void *)&Token::isBreak);
    tokenType->addMethod(mod->getBoolType(), "isClass", (void *)&Token::isClass);
    tokenType->addMethod(mod->getBoolType(), "isContinue", (void *)&Token::isContinue);
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

    Type *cc = mod->addType("CrackContext");
    f = cc->addMethod(mod->getVoidType(), "inject",
                      (void *)&CrackContext::inject
                      );
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

    cc->finish();
    
    // our annotations
    f = mod->addFunc(mod->getVoidType(), "static", (void *)staticAnn);
    f->addArg(cc, "ctx");
    f = mod->addFunc(mod->getVoidType(), "final", (void *)finalAnn);
    f->addArg(cc, "ctx");
    
    // constants
    mod->addConstant(mod->getIntType(), "SCOPE_MODULE", 0);
    mod->addConstant(mod->getIntType(), "SCOPE_FUNCTION", 2);
    mod->addConstant(mod->getIntType(), "SCOPE_CLASS", 3);
    mod->addConstant(mod->getIntType(), "STATE_BASE", parser::Parser::st_base);
    mod->addConstant(mod->getIntType(), "FUNC_ENTER", 
                     parser::Parser::funcEnter
                     );
    mod->addConstant(mod->getIntType(), "FUNC_LEAVE",
                     parser::Parser::funcLeave
                     );
    
    mod->addConstant(mod->getIntType(), "FUNCFLAG_STATIC",
                     model::FuncDef::explicitFlags
                     );
    mod->addConstant(mod->getIntType(), "FUNCFLAG_FINAL",
                     model::FuncDef::explicitFlags | model::FuncDef::method
                     );
                    
}

} // namespace compiler
