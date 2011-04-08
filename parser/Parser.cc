// Copyright 2009 Google Inc., Shannon Weyrick <weyrick@mozek.us>

#include "Parser.h"

#include <assert.h>
#include <sstream>
#include <stdexcept>
#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include "model/Annotation.h"
#include "model/ArgDef.h"
#include "model/AssignExpr.h"
#include "model/Branchpoint.h"
#include "model/CompositeNamespace.h"
#include "model/CleanupFrame.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/FuncCall.h"
#include "model/Expr.h"
#include "model/Initializers.h"
#include "model/IntConst.h"
#include "model/FloatConst.h"
#include "model/ModuleDef.h"
#include "model/NullConst.h"
#include "model/ResultExpr.h"
#include "model/StrConst.h"
#include "model/StubDef.h"
#include "model/TypeDef.h"
#include "model/OverloadDef.h"
#include "model/VarDef.h"
#include "model/VarRef.h"
#include "builder/Builder.h"
#include "ParseError.h"
#include <cstdlib>
#include <stdint.h>

using namespace std;
using namespace parser;
using namespace model;

void Parser::addDef(VarDef *varDef) {
   FuncDef *func = FuncDefPtr::cast(varDef);
   ContextPtr defContext = context->getDefContext();
   VarDefPtr storedDef = defContext->addDef(varDef);
   
   // if this was a function that was added to an ancestor context, we need to 
   // rebuild any intermediate overload definitions.
   if (func && defContext != context)
      context->insureOverloadPath(defContext.get(), 
                                  OverloadDefPtr::arcast(storedDef)
                                  );
   
   // if the definition context is a class context and the definition is a 
   // function and this isn't the "Class" class (which is its own meta-class), 
   // add it to the meta-class.
   TypeDef *type;
   if (defContext->scope == Context::instance && func) {
      type = TypeDefPtr::arcast(defContext->ns);
      if (type != type->type.get())
         type->type->addAlias(storedDef.get());
   }
}

void Parser::addFuncDef(FuncDef *funcDef) {
   addDef(funcDef);
}

Token Parser::getToken() {
   Token tok = toker.getToken();
   context->setLocation(tok.getLocation());
   
   // short-circuit the parser for an annotation, which can occur anywhere.
   while (tok.isAnn() || tok.getType() == Token::popErrCtx) {
      if (tok.isAnn())
         parseAnnotation();
      else
         context->popErrorContext();
      tok = toker.getToken();
      context->setLocation(tok.getLocation());
   }

   return tok;
}

unsigned Parser::getPrecedence(const string &op) {
   OpPrecMap::iterator iter = opPrecMap.find(op);
   assert(iter != opPrecMap.end() && "got operator with no precedence");
   return iter->second;
}

void Parser::unexpected(const Token &tok, const char *userMsg) {
   Location loc = tok.getLocation();
   stringstream msg;
   msg << "token " << tok.getData() << " was not expected at this time";

   // add the user message, if provided
   if (userMsg)
      msg << ", " << userMsg;

   error(tok, msg.str());
}

void Parser::expectToken(Token::Type type, const char *error) {
   Token tok = getToken();
   if (tok.getType() != type)
      unexpected(tok, error);
}

FuncDefPtr Parser::lookUpBinOp(const string &name, FuncCall::ExprVec &args) {
   FuncCall::ExprVec exprs(1);
   exprs[0] = args[1];
   
   // first try to find it in the type's context, then try to find it in 
   // the current context.
   FuncDefPtr func = context->lookUp(name, exprs, args[0]->type.get());
   if (!func) {
      exprs[0] = args[0];
      exprs.push_back(args[1]);
      func = context->lookUp(name, exprs);
   }

   args = exprs;
   return func;
}   

void Parser::parseClause(bool defsAllowed) {
   Token tok = getToken();
   state = st_notBase;
   ExprPtr expr;
   if (tok.isIdent()) {
      
      // if the identifier is a type, try to parse a definition
      VarDefPtr def = context->ns->lookUp(tok.getData());
      TypeDef *typeDef = TypeDefPtr::rcast(def);
      if (typeDef && parseDef(typeDef)) {
         if (!defsAllowed)
            error(tok, "definition is not allowed in this context");
         // bypass expression emission and semicolon parsing (parseDef() 
         // consumes it's own semicolon)
         return;
      } else if (typeDef) {
         // we didn't parse a definition
         
         // see if this is a define
         Token tok2 = getToken();
         if (tok2.isDefine()) {
            if (def->getOwner() == context->ns.get())
               redefineError(tok2, def.get());
            
            expr = parseExpression();
            context->emitVarDef(expr->type.get(), tok, expr.get());
            
            // don't do expression processing
            expr = 0;
         } else {

            // try treating the class as a primary
            toker.putBack(tok2);
            expr = parsePostIdent(0, tok);
            expr = parseSecondary(expr.get());
         }
      } else if (!def) {
         // XXX think I want to move this into expression, it's just as valid 
         // for an existing identifier (in a parent context) as for a 
         // non-existing one.
         // unknown identifier. if the next token(s) is ':=' (the "define" 
         // operator) then this is an assignment
         Token tok2 = getToken();
         if (tok2.isDefine()) {
            if (!defsAllowed)
               error(tok, "definition is not allowed in this context.");
            expr = parseExpression();
            context->emitVarDef(expr->type.get(), tok, expr.get());
            
            // trick the expression processing into not happening
            expr = 0;
         } else {
            error(tok, SPUG_FSTR("Unknown identifier " << tok.getData()));
         }
      } else {
         toker.putBack(tok);
         runCallbacks(exprBegin);
         expr = parseExpression();
      }

   } else {
      toker.putBack(tok);
      runCallbacks(exprBegin);
      expr = parseExpression();
   }

   // if we got an expression, emit it.
   if (expr) {
      context->createCleanupFrame();
      expr->emit(*context)->handleTransient(*context);
      context->closeCleanupFrame();
   }

   // consume a semicolon, put back a block terminator
   tok = getToken();
   if (tok.isEnd() || tok.isRCurly())
      toker.putBack(tok);
   else if (!tok.isSemi())
      unexpected(tok, "expected semicolon or a block terminator");
}

void Parser::parseAnnotation() {
   AnnotationPtr ann;
   {
      // create a new context whose construct is tha annotation construct.
      ContextPtr parentContext = context;
      ContextPtr ctx = context->createSubContext(Context::module);
      ContextStackFrame cstack(*this, ctx.get());
      context->construct = context->getCompileTimeConstruct();
   
      Token tok = toker.getToken();
      context->setLocation(tok.getLocation());
   
      // if we get an import keyword, parse the import statement.   
      if (tok.isImport()) {
         parseImportStmt(parentContext->compileNS.get());
         return;
      }
      
      if (!tok.isIdent())
         error(tok, "Identifier or import statement expected after '@' sign");
      
      // lookup the annotation
      ann = context->lookUpAnnotation(tok.getData());
      if (!ann)
         error(tok, SPUG_FSTR("Undefined annotation " << tok.getData()));
   }
   
   // invoke in the outer context
   ann->invoke(this, &toker, context.get());
}

ContextPtr Parser::parseStatement(bool defsAllowed) {
   // peek at the next token
   Token tok = getToken();
   state = st_notBase;

   // check for statements
   if (tok.isSemi()) {
      // null statement
      runCallbacks(controlStmt);
      return 0;
   } else if (tok.isIf()) {
      runCallbacks(controlStmt);
      return parseIfStmt();
   } else if (tok.isWhile()) {
      runCallbacks(controlStmt);
      parseWhileStmt();

      // while statements are never terminal, there's always the possibility 
      // that we could never execute the body.
      return 0;
   } else if (tok.isElse()) {
      unexpected(tok, "'else' with no matching 'if'");
   } else if (tok.isReturn()) {
      runCallbacks(controlStmt);
      if (context->scope == Context::module)
         error(tok, "Return statement not allowed in module scope");
      parseReturnStmt();
      return context->getToplevel()->getParent();
   } else if (tok.isImport()) {
      runCallbacks(controlStmt);
      parseImportStmt(context->ns.get());
      return 0;
   } else if (tok.isClass()) {
      if (!defsAllowed)
         error(tok, "class definitions are not allowed in this context");
      parseClassDef();
      return 0;      
   } else if (tok.isBreak()) {
      runCallbacks(controlStmt);
      Branchpoint *branch = context->getBreak();
      if (!branch)
         error(tok, 
               "Break can only be used in the body of a while, for or "
               "switch statement."
               );
      context->builder.emitBreak(*context, branch);
      
      tok = getToken();
      if (!tok.isSemi())
         toker.putBack(tok);
      assert(branch->context);
      return branch->context;
   } else if (tok.isContinue()) {
      runCallbacks(controlStmt);
      Branchpoint *branch = context->getContinue();
      if (!branch)
         error(tok,
               "Continue can only be used in the body of a while or for "
               "loop."
               );
      context->builder.emitContinue(*context, branch);

      tok = getToken();
      if (!tok.isSemi())
         toker.putBack(tok);
      assert(branch->context);
      return branch->context;
   } else if (tok.isFor()) {
      parseForStmt();
      
      // for statements are like while - never terminal
      return 0;
   } else if (tok.isThrow()) {
      return parseThrowStmt();
   } else if (tok.isTry()) {
      return parseTryStmt();
   }

   toker.putBack(tok);
   state = st_base;
   parseClause(defsAllowed);

   return 0;
}

ContextPtr Parser::parseBlock(bool nested, Parser::Event closeEvent) {
   Token tok;
   ContextPtr terminal;

   // keeps track of whether we've emitted a warning about stuff after a 
   // terminal statement.
   bool gotStuffAfterTerminalStatement = false;

   while (true) {
      state = st_base;

      // peek at the next token
      tok = getToken();

      // check for a different block terminator depending on whether we are
      // nested or not.
      bool gotBlockTerminator = false;
      if (tok.isRCurly()) {
         if (!nested)
            unexpected(tok, "expected statement or end-of-file.");
         gotBlockTerminator = true;
      } else if (tok.isEnd()) {
         if (nested)
             unexpected(tok, "expected statement or closing brace.");
         gotBlockTerminator = true;
      }
      
      if (gotBlockTerminator) {
         // if there are callbacks, we have to put the last token back and 
         // then run the callbacks and then make sure that we got the same
         // terminator again (since the callbacks can insert tokens into the 
         // stream).
         if (callbacks[closeEvent].size()) {
            toker.putBack(tok);
            runCallbacks(closeEvent);
            Token tempTok = toker.getToken();
            if (!tempTok.isRCurly()) {
               // if the token is not what it was before, one of the callbacks 
               // has changed the token stream and we need to go back to the 
               // loop.
               toker.putBack(tempTok);
               continue;
            }
         }

         // make sure that the context contains no forward declarations
         context->checkForUnresolvedForwards();

         // generate all of the cleanups, but not if we already did this (got 
         // a terminal statement) or we're at the top-level module (in which 
         // case we'll want to generate cleanups in a static cleanup function)
         if (!context->terminal && nested)
            context->builder.closeAllCleanups(*context);
         return terminal;
      }
      
      toker.putBack(tok);
      
      // if we already got a terminal statement, anything else is just dead 
      // code - warn them about the first thing. TODO: convert this to a 
      // warning once we can turn off code generation.
      if (context->terminal && !gotStuffAfterTerminalStatement) {
         error(tok, "unreachable code");
         gotStuffAfterTerminalStatement = true;
      }

      terminal = parseStatement(true);
      if (terminal)
         if (terminal != context)
            context->terminal = true;
         else
            terminal = 0;
   }
}

ExprPtr Parser::createVarRef(Expr *container, VarDef *var, const Token &tok) {
   // if the definition is for an instance variable, emit an implicit 
   // "this" dereference.  Otherwise just emit the variable
   if (TypeDefPtr::cast(var->getOwner())) {
      // if there's no container, try to use an implicit "this"
      ExprPtr receiver = container ? container : 
                                     context->makeThisRef(tok.getData());
      return context->createFieldRef(receiver.get(), var);
   } else {
      return context->createVarRef(var);
   }
}

ExprPtr Parser::createVarRef(Expr *container, const Token &ident) {
   Namespace &varNS = container ? *container->type : *context->ns;
   VarDefPtr var = varNS.lookUp(ident.getData());
   if (!var)
      error(ident,
            SPUG_FSTR("Undefined variable: " << ident.getData()));
   
   // check for an overload definition - if it is one, make sure there's only 
   // a single overload.
   OverloadDef *ovld = OverloadDefPtr::rcast(var);
   if (ovld) {
      if (!ovld->isSingleFunction())
         error(ident, 
               SPUG_FSTR("Cannot reference function " << ident.getData() <<
                          " because there are multiple overloads."
                         )
               );
      
      // make sure that the implementation has been defined
      ovld->createImpl();
   }

   return createVarRef(container, var.get(), ident);
}

ExprPtr Parser::createAssign(Expr *container, const Token &ident,
                             VarDef *var,
                             Expr *val
                             ) {
   if (TypeDefPtr::cast(var->getOwner())) {
      // if there's no container, try to use an implicit "this"
      ExprPtr receiver = container ? container : 
                                     context->makeThisRef(ident.getData());
      return AssignExpr::create(*context, receiver.get(), var, val);
   } else {
      return AssignExpr::create(*context, var, val);
   }
}

// obj.oper <symbol>
string Parser::parseOperSpec() {
   Token tok = getToken();
   const string &ident = tok.getData();
   if (tok.isMinus() || tok.isTilde() || tok.isBang() ||
       tok.isEQ() || tok.isNE() || tok.isLT() || tok.isLE() || 
       tok.isGE() || tok.isGT() || tok.isPlus() || tok.isSlash() || 
       tok.isAsterisk() || tok.isPercent() ||
       ident == "init" || ident == "release" || ident == "bind" ||
       ident == "del" || tok.isAugAssign()
       ) {
      return "oper " + ident;
   } else if (tok.isIncr() || tok.isDecr()) {
      
      // make sure the next token is an "x"
      Token tok2 = getToken();
      if (!tok2.isIdent() || tok2.getData() != "x")
         unexpected(tok2, 
                    SPUG_FSTR("expected an 'x' after oper " << ident).c_str()
                    );

      return "oper " + ident + "x";
   } else if (ident == "x") {
      tok = getToken();
      if (tok.isIncr() || tok.isDecr())
         return "oper x" + tok.getData();
      else
         unexpected(tok, 
                    "Expected an increment or decrement operator after oper x."
                    );
                    
   } else if (tok.isLBracket()) {
      tok = getToken();
      if (!tok.isRBracket())
         error(tok, "Expected right bracket in 'oper ['");
      
      // see if this is "[]="
      tok = getToken();
      if (tok.isAssign()) {
         return "oper []=";
      } else {
         toker.putBack(tok);
         return "oper []";
      }
   } else if (ident == "to") {
      TypeDefPtr type = parseTypeSpec();
      return "oper to " + type->getFullName();
   } else {
      unexpected(tok, "expected legal operator name or symbol.");
   }
}

FuncCallPtr Parser::parseFuncCall(const Token &ident, const string &funcName,
                                  Namespace *ns, 
                                  Expr *container
                                  ) {

   // parse the arg list
   FuncCall::ExprVec args;
   parseMethodArgs(args);
   
   // look up the variable
   
   // lookup the method from the variable context's type context
   // if the container is a class, assume that this is a lookup in a specific 
   // base class and allow looking up overrides for it (this won't work if we 
   // ever give class objects a vtable because it will break meta-class 
   // methods, but we need to replace the specific lookup syntax anyway)
   // XXX needs to handle callable objects.
   FuncDefPtr func = context->lookUp(funcName, args, ns, 
                                     container && container->type->meta
                                     );
   if (!func)
      error(ident, SPUG_FSTR("No method exists matching " << funcName << 
                              "(" << args << ")"));

   // if the definition is for an instance variable, emit an implicit 
   // "this" dereference.  Otherwise just emit the variable
   ExprPtr receiver;
   bool squashVirtual = false;
   if (func->flags & FuncDef::method) {
      // keep track of whether we need to verify that "this" is an instance 
      // of the container (assumes the container is a TypeDef)
      bool verifyThisIsContainer = false;

      // if we've got a container and the container is not a class, or the 
      // container _is_ a class but the function is a method of its 
      // meta-class, use the container as the receiver.
      if (container)
         if (container->type->meta && !TypeDefPtr::acast(func->getOwner())->meta) {
            // the container is a class and the function is an explicit 
            // call of a (presumably base class) method.
            squashVirtual = true;
            verifyThisIsContainer = true;
         } else {
            receiver = container;
         }

      // if we didn't get the receiver from the container, lookup the 
      // "this" variable.
      if (!receiver) {
         receiver = context->makeThisRef(funcName);
         if (verifyThisIsContainer && 
              !receiver->type->isDerivedFrom(container->type->meta))
            error(ident, SPUG_FSTR("'this' is not an instance of " <<
                                    container->type->meta->name
                                   )
                  );
      }
   }

   FuncCallPtr funcCall = context->builder.createFuncCall(func.get(),
                                                          squashVirtual
                                                          );
   funcCall->args = args;
   funcCall->receiver = receiver;
   return funcCall;
}

ExprPtr Parser::parsePostIdent(Expr *container, const Token &ident) {
   Namespace *ns = container ? container->type.get() : context->ns.get();

   Token tok1 = getToken();
   if (ident.isOper() && tok1.isAssign())
      error(tok1, "Expected operator identifier after 'oper' keyword");

   // is it an assignment?
   if ((tok1.isAssign() || tok1.isAugAssign()) && !ident.isOper()) {
      
      VarDefPtr var = ns->lookUp(ident.getData());
      if (!var)
         error(tok1,
               SPUG_FSTR("attempted to assign undefined variable " <<
                          ident.getData()
                         )
               );

      // make sure the variable is not a constant.
      if (var->isConstant())
         error(tok1, "You cannot assign to a constant, class or function.");

      // parse an expression
      ExprPtr val = parseExpression();
      if (!val) {
         tok1 = getToken();
         error(tok1, "expression expected");
      }
      
      // check for augmented assignment
      if (tok1.isAugAssign()) {
         
         // create a reference for the lvalue
         ExprPtr varRef = createVarRef(container, var.get(), ident);

         // see if the variable's type has an augmented assignment operator
         FuncCall::ExprVec args(2);
         args[0] = varRef;
         args[1] = val;
         FuncDefPtr funcDef;
         if (funcDef = lookUpBinOp("oper " + tok1.getData(), args)) {
            FuncCallPtr funcCall =
               context->builder.createFuncCall(funcDef.get());
            funcCall->args = args;
            if (funcDef->method)
               funcCall->receiver = varRef;
            return funcCall;
         }
         
         // it doesn't.  verify that it has the plain version of the operator 
         // and construct an assignment from it.
         args[0] = varRef;
         args[1] = val;
         const string &tok1Data = tok1.getData();
         const string &oper = tok1Data.substr(0, tok1Data.size() - 1);
         if (funcDef = lookUpBinOp("oper " + oper, args)) {
            FuncCallPtr funcCall = 
               context->builder.createFuncCall(funcDef.get());
            funcCall->args = args;
            if (funcDef->method)
               funcCall->receiver = varRef;
            return createAssign(container, ident, var.get(), funcCall.get());
         } else {
            error(tok1, SPUG_FSTR("Neither " << oper << "=  nor " << oper << 
                                  " is defined for types " << 
                                  varRef->type->name << " and " <<
                                  val->type->name
                                  )
                  );
         }
      }

      // if this is an instance variable, emit a field assignment.  
      // Otherwise emit a normal variable assignment.
      return createAssign(container, ident, var.get(), val.get());
   } // should not fall through - always returns or throws.

   // if this is an explicit operator call, give it special treatment.
   string funcName;
   if (ident.isOper()) {
      toker.putBack(tok1);
      funcName = parseOperSpec();
      tok1 = getToken();
   } else {
      funcName = ident.getData();
   }
   
   if (tok1.isLParen()) {
      // function/method invocation
      return parseFuncCall(ident, funcName, ns, container);
   } else {
      if (ident.isOper())
         unexpected(tok1,
                    SPUG_FSTR("expected parameter list after " << 
                              funcName
                              ).c_str()
                    );

      // for anything else, it's a variable reference
      toker.putBack(tok1);
      return createVarRef(container, ident);
   }

}

// ` ... `
//  ^
ExprPtr Parser::parseIString(Expr *expr) {
   // emit the expression, all subsequent invocations will use the result
   ResultExprPtr result = expr->emit(*context);
   context->createCleanupFrame();
   result->handleTransient(*context);
   
   // put the whole thing in an "if"
   TypeDef *boolType = context->construct->boolType.get();
   ExprPtr cond = result->convert(*context, boolType);
   if (!cond)
      context->error("interpolated string target can not be converted to a "
                      "bool.");
   BranchpointPtr pos = context->builder.emitIf(*context, cond.get());
   
   // parse all of the subtokens
   Token tok;
   while (!(tok = getToken()).isIstrEnd()) {
      ExprPtr arg;
      if (tok.isString()) {
         arg = context->getStrConst(tok.getData());
      } else if (tok.isIdent()) {
         // get a variable definition
         arg = createVarRef(0, tok);
         toker.continueIString();
      } else if (tok.isLParen()) {
         arg = parseExpression();
         tok = getToken();
         if (!tok.isRParen())
            unexpected(tok, "expected a right paren");
         toker.continueIString();
      } else {
         unexpected(tok, 
                    "expected an identifer or a parenthesized expression "
                     "after the $ in an interpolated string"
                    );
      }

      // look up a format method for the argument
      FuncCall::ExprVec args(1);
      args[0] = arg;
      FuncDefPtr func = context->lookUp("format", args, expr->type.get());
      if (!func)
         error(tok, 
               SPUG_FSTR("No format method exists for objects of type " <<
                         arg->type->getFullName()
                         )
               );
      
      FuncCallPtr funcCall = context->builder.createFuncCall(func.get());
      funcCall->args = args;
      funcCall->receiver = result;
      funcCall->emit(*context);
   }
   
   context->closeCleanupFrame();
   context->builder.emitEndIf(*context, pos.get(), false);
   
   return result;
}

// [ expr, expr, ... ]
//  ^                 ^
ExprPtr Parser::parseConstSequence(TypeDef *containerType) {
   vector<ExprPtr> elems;
   Token tok = toker.getToken();
   while (!tok.isRBracket()) {
      // parse the next element
      toker.putBack(tok);
      elems.push_back(parseExpression());

      tok = toker.getToken();
      if (tok.isComma())
         tok = toker.getToken();
      else if (!tok.isRBracket())
         unexpected(tok, "Expected comma or right bracket after element");
   }
   
   return context->emitConstSequence(containerType, elems);
}
   

TypeDef *Parser::convertTypeRef(Expr *expr) {
   VarRef *ref = VarRefPtr::cast(expr);
   if (!ref)
      return 0;
   
   return TypeDefPtr::rcast(ref->def);
}

// cond ? trueVal : falseVal
//       ^                  ^
ExprPtr Parser::parseTernary(Expr *cond) {
   ExprPtr trueVal = parseExpression();
   Token tok = getToken();
   if (!tok.isColon())
      unexpected(tok, "expected colon.");
   ExprPtr falseVal = parseExpression();
   return context->createTernary(cond, trueVal.get(), falseVal.get());
}

ExprPtr Parser::parseSecondary(Expr *expr0, unsigned precedence) {
   ExprPtr expr = expr0;
   Token tok = getToken();
   while (true) {
      if (tok.isDot()) {
	 tok = getToken();
	 
	 // if the next token is "class", this is the class operator.
	 if (tok.isClass()) {
            FuncDefPtr funcDef = 
               context->lookUpNoArgs("oper class", true, expr->type.get());
            if (!funcDef)
               error(tok, SPUG_FSTR("class operator not defined for " <<
                                    expr->type->name
                                    )
                     );
            
            FuncCallPtr funcCall =
               context->builder.createFuncCall(funcDef.get());
            funcCall->receiver = expr;
            expr = funcCall;
            tok = getToken();
            continue;
         
	 } else if (!tok.isIdent() && !tok.isOper()) {
            // make sure it's an identifier
            error(tok, "identifier expected");
	 }

         expr = parsePostIdent(expr.get(), tok);
      } else if (tok.isLBracket()) {
         // the array indexing operators
         
         // ... unless this is a type, in which case it is a specializer.
         TypeDef *generic = convertTypeRef(expr.get());
         if (generic) {
            TypeDef *type = parseSpecializer(tok, generic);
            
            // check for a constructor
            tok = getToken();
            if (tok.isLParen()) {
               expr = parseConstructor(tok, type, Token::rparen);
               tok = getToken();
            } else {
               // otherwise just create a reference to the type.
               expr = context->createVarRef(type);
            }
            continue;
         }
         
         FuncCall::ExprVec args(1);
         args[0] = parseExpression();
         
         // parse the right bracket
         Token tok2 = getToken();
         if (!tok2.isRBracket())
            unexpected(tok2, "expected right bracket.");
         
         // check for an assignment operator
         tok2 = getToken();
         FuncCallPtr funcCall;
         if (tok2.isAssign()) {
            // this is "a[i] = v"
            args.push_back(parseExpression());
            FuncDefPtr funcDef =
               context->lookUp("oper []=", args, expr->type.get());
            if (!funcDef)
               error(tok, 
                     SPUG_FSTR("'oper []=' not defined for " <<
                               expr->type->name << " with these arguments."
                               )
                     );
            funcCall = context->builder.createFuncCall(funcDef.get());
            funcCall->receiver = expr;
            funcCall->args = args;
         } else {
            // this is "a[i]"
            toker.putBack(tok2);
            FuncDefPtr funcDef =
               context->lookUp("oper []", args, expr->type.get());
            if (!funcDef)
               error(tok, SPUG_FSTR("'oper []' not defined for " <<
                                     expr->type->name << 
                                     " with these arguments: (" << args << ")"
                                    )
                     );
            funcCall = context->builder.createFuncCall(funcDef.get());
            funcCall->receiver = expr;
            funcCall->args = args;
         }
         
         expr = funcCall;
      
      } else if (tok.isIncr() || tok.isDecr()) {
         
         FuncCall::ExprVec args;
         string symbol = "oper x" + tok.getData();
         FuncDefPtr funcDef = context->lookUp(symbol, args, expr->type.get());
         if (!funcDef) {
            args.push_back(expr);
            funcDef = context->lookUp(symbol, args);
         }
         if (!funcDef)
            error(tok, SPUG_FSTR(symbol << " is not defined for type "
                                        << expr->type->name));
   
         FuncCallPtr funcCall = context->builder.createFuncCall(funcDef.get());
         funcCall->args = args;
         if (funcDef->flags & FuncDef::method)
            funcCall->receiver = expr;
         expr = funcCall;
         
      } else if (tok.isBinOp()) {
         // get the precedence of the new operator, if it's lower than the 
         // or the same as that of the current operator, quit.
         unsigned newPrec = getPrecedence(tok.getData());
         if (newPrec <= precedence)
            break;

         // parse the right-hand-side expression
         ExprPtr rhs = parseExpression(newPrec);
         
         FuncCall::ExprVec exprs(2);
         exprs[0] = expr;
         exprs[1] = rhs;
         std::string name = "oper " + tok.getData();

         FuncDefPtr func = lookUpBinOp(name, exprs);
         if (!func)
            error(tok,
                  SPUG_FSTR("Operator " << expr->type->name << " " <<
                            tok.getData() << " " << rhs->type->name <<
                            " undefined."
                            )
                  );
         FuncCallPtr funcCall = context->builder.createFuncCall(func.get());
         funcCall->args = exprs;
         if (func->flags & FuncDef::method)
            funcCall->receiver = expr;
         expr = funcCall;
      } else if (tok.isIstrBegin()) {
         expr = parseIString(expr.get());
      } else if (tok.isQuest()) {
         if (precedence >= logOrPrec)
            break;
         expr = parseTernary(expr.get());
      } else if (tok.isBang()) {
         // this is special wacky collection syntax Type![1, 2, 3]
         TypeDef *type = convertTypeRef(expr.get());
         if (!type)
            error(tok, 
                  "Exclamation point can not follow a non-type expression"
                  );
         
         // check for a square bracket
         tok = toker.getToken();
         if (!tok.isLBracket())
            error(tok,
                  "Sequence initializer ('[ ... ]') expected after "
                   "'type!'"
                  );
         expr = parseConstSequence(type);
      } else {
	 // next token is not part of the expression
	 break;
      }

      // get the next token
      tok = getToken();

   }
   toker.putBack(tok);
   return expr;
}   

namespace {

   ExprPtr parseConstInt(Context &context,
                        bool negative,
                        const string &val,
                        int base) {
      if (negative) {
         // we need to give strtoll the - in the string, because otherwise the
         // range is off and we can't parse LONG_MIN
         string nval = "-"+val;
         return context.builder.createIntConst(context,
                                               strtoll(nval.c_str(), NULL,
                                                       base
                                                       )
                                               );
      }
      
      // if the constant starts with an 'i' or 'b', this is a string whose 
      // bytes comprise the integer value.
      if (val[0] == 'b') {
         if (val.size() != 2)
            context.error("Byte constants from strings must be exactly one "
                           "byte long."
                          );
         TypeDef *byteType = context.construct->byteType.get();
         return context.builder.createIntConst(context, val[1], byteType);
      } else if (val[0] == 'i') {
         if (val.size() < 2 || val.size() > 9)
            context.error("Integer constants from strings must be between one "
                           "and 8 bytes long"
                          );

         // construct an integer from the bytes in the string
         int64_t n = 0;
         for (int i = 1; i < val.size(); ++i)
            n = n << 8 | val[i];

         return context.builder.createIntConst(context, n);
      }
   
      // if it's not negative, we first try to parse it as unsigned
      // if it's small enough to fit in a signed, we do that, otherwise
      // we keep it unsigned
      unsigned long long bigcval = strtoull(val.c_str(), NULL, base);
      if (bigcval <= INT64_MAX) {
         // signed
         return context.builder.createIntConst(context,
                                               strtoll(val.c_str(),
                                                       NULL,
                                                       base
                                                       )
                                               );
      } else {
         // unsigned
         return context.builder.createUIntConst(context, bigcval);
      }
   }

} // anonymous namespace

ExprPtr Parser::parseExpression(unsigned precedence, bool unaryMinus) {

   ExprPtr expr;

   // check for null
   Token tok = getToken();
   if (tok.isNull()) {
      expr = new NullConst(context->construct->voidptrType.get());
   
   // check for a nested parenthesized expression
   } else if (tok.isLParen()) {
      expr = parseExpression();
      tok = getToken();
      if (!tok.isRParen())
         unexpected(tok, "expected a right paren");

   // check for a method
   } else if (tok.isIdent()) {
      expr = parsePostIdent(0, tok);
   
   // for a string constant
   } else if (tok.isString()) {
      
      // check for subsequent string constants, concatenate them.
      ostringstream result;
      while (tok.isString()) {
         result << tok.getData();
         tok = getToken();
      }
      toker.putBack(tok);
      expr = context->getStrConst(result.str());
   
   // for an interpolated string
   } else if (tok.isIstrBegin()) {
      assert(false && "istring expressions not yet implemented");
      // XXX we need to create a StringFormatter and pass it to parseIString() 
      // as the formatter.
//      expr = parseIString(formatter);
   
   // for a numeric constants
   } else if (tok.isInteger()) {
      expr = parseConstInt(*context, unaryMinus, tok.getData(), 10);
   } else if (tok.isFloat()) {
      // XXX use unaryMinus?
      expr = context->builder.createFloatConst(*context,
                                               atof(tok.getData().c_str())
                                               );

   } else if (tok.isOctal()) {
      expr = parseConstInt(*context, unaryMinus, tok.getData(), 8);
   } else if (tok.isHex()) {
      expr = parseConstInt(*context, unaryMinus, tok.getData(), 16);
   } else if (tok.isBinary()) {
      expr = parseConstInt(*context, unaryMinus, tok.getData(), 2);
   } else if (tok.isPlus()) {
       // eat + if expression is a numeric constant and fail if it's not
       tok = getToken();
       if (tok.isInteger())
           expr = parseConstInt(*context, unaryMinus, tok.getData(), 10);
       else if(tok.isFloat())
           expr = context->builder.createFloatConst(*context,
                                                    atof(tok.getData().c_str())
                                                    );
       else if (tok.isOctal())
           expr = parseConstInt(*context, unaryMinus, tok.getData(), 8);
       else if (tok.isHex())
           expr = parseConstInt(*context, unaryMinus, tok.getData(), 16);
       else if (tok.isBinary())
           expr = parseConstInt(*context, unaryMinus, tok.getData(), 2);
       else
           unexpected(tok, "unexpected unary +");
   // for the unary operators
   } else if (tok.isBang() || tok.isMinus() || tok.isTilde() ||
              tok.isDecr() || tok.isIncr()) {
      FuncCall::ExprVec args;
      string symbol = tok.getData();

      // parse the expression
      ExprPtr operand = parseExpression(getPrecedence(symbol + "x"),
                                        tok.isMinus());


      // if this is a minus, see if the expression is an integer constant and 
      // simply negate it if it is.
      IntConst *ic;
      if (tok.isMinus() && (ic = IntConstPtr::rcast(operand))) {
         // nop, handled above with unaryMinus in tok.isInteger() et al
         expr = operand;
      } else {

         // try to look it up for the expression, then for the context.
         symbol = "oper " + symbol;
         if (tok.isIncr() || tok.isDecr())
            symbol += "x";
         FuncDefPtr funcDef = context->lookUp(symbol, args, 
                                              operand->type.get()
                                              );
         if (!funcDef) {
            args.push_back(operand);
            funcDef = context->lookUp(symbol, args);
         }
         if (!funcDef)
            error(tok, SPUG_FSTR(symbol << " is not defined for type "
                                        << operand->type->name));

   
         FuncCallPtr funcCall = context->builder.createFuncCall(funcDef.get());
         funcCall->args = args;
         if (funcDef->flags & FuncDef::method)
            funcCall->receiver = operand;
         expr = funcCall;
      }
   } else if (tok.isLCurly()) {
      assert(false);
   } else {
      unexpected(tok, "expected an expression");
   }

   return parseSecondary(expr.get(), precedence);
}

// func( arg, arg)
//      ^         ^
// Type var = { arg, arg } ;
//             ^          ^
void Parser::parseMethodArgs(FuncCall::ExprVec &args, Token::Type terminator) {
     
   Token tok = getToken();
   while (true) {
      if (tok.getType() == terminator)
         return;
         
      // XXX should be verifying arg types against signature

      // get the next argument value
      toker.putBack(tok);
      ExprPtr arg = parseExpression();
      args.push_back(arg);

      // comma signals another argument
      tok = getToken();
      if (tok.isComma())
         tok = getToken();
   }
}

// type [ subtype, ... ]
//       ^              ^
TypeDef *Parser::parseSpecializer(const Token &lbrack, TypeDef *typeDef) {
   if (!typeDef->generic)
      error(lbrack, 
            SPUG_FSTR("You cannot specialize non-generic type " <<
                       typeDef->getFullName()
                      )
            );
   
   TypeDef::TypeVecObjPtr types = new TypeDef::TypeVecObj();
   Token tok;
   while (true) {      
      TypeDefPtr subType = parseTypeSpec();
      types->push_back(subType);
      
      tok = getToken();
      if (tok.isRBracket())
         break;
      else if (!tok.isComma())
         error(tok, "comma expected in specializer list.");
   }

   // XXX needs to verify the numbers and types of specializers
   typeDef = typeDef->getSpecialization(*context, types.get());
   return typeDef;
}

// Class( arg, arg )
//       ^          ^
// Class var = { arg, arg } ;
//              ^          ^
ExprPtr Parser::parseConstructor(const Token &tok, TypeDef *type,
                                 Token::Type terminator
                                 ) {
   // parse an arg list
   FuncCall::ExprVec args;
   parseMethodArgs(args, terminator);
   
   // look up the new operator for the class
   FuncDefPtr func = context->lookUp("oper new", args, type);
   if (!func)
      error(tok, SPUG_FSTR("No constructor for " << type->name <<
                           " with these argument types: (" << args << ")"));
   
   FuncCallPtr funcCall = context->builder.createFuncCall(func.get());
   funcCall->args = args;
   return funcCall;
}

TypeDefPtr Parser::parseTypeSpec(const char *errorMsg) {
   Token tok = getToken();
   if (!tok.isIdent())
      unexpected(tok, "type identifier expected");
   
   VarDefPtr def = context->ns->lookUp(tok.getData());
   TypeDef *typeDef = TypeDefPtr::rcast(def);
   if (!typeDef)
      error(tok, SPUG_FSTR(tok.getData() << errorMsg));
   
   // see if there's a bracket operator   
   tok = getToken();
   if (tok.isLBracket())
      typeDef = parseSpecializer(tok, typeDef);
   else
      toker.putBack(tok);
   
   // make sure this isn't an unspecialized generic
   if (typeDef->generic)
      error(tok, SPUG_FSTR("Generic type " << typeDef->name << 
                            " must be specialized to be used."
                           )
            );
   
   return typeDef;
}

void Parser::parseModuleName(vector<string> &moduleName) {
   Token tok = getToken();
   while (true) {
      moduleName.push_back(tok.getData());
      tok = getToken();
      if (!tok.isDot()) {
         toker.putBack(tok);
         return;
      }
      
      tok = getToken();
      if (!tok.isIdent())
         unexpected(tok, "identifier expected");
   }
}

// type funcName ( type argName, ... ) {
//                ^                   ^
void Parser::parseArgDefs(vector<ArgDefPtr> &args, bool isMethod) {

   // load the next token so we can check for the immediate closing paren of 
   // an empty argument list.
   Token tok = getToken();
      
   while (!tok.isRParen()) {

      // parse the next argument type
      toker.putBack(tok);
      TypeDefPtr argType = parseTypeSpec();
      
      tok = getToken();
      if (!tok.isIdent())
         error(tok, "identifier (argument name) expected.");
      
      // make sure we're not redefining an existing variable
      // XXX complain extra loud if the variable is defined in the current 
      // context (and make sure that the current context is the function 
      // context)
      std::string varName = tok.getData();
      checkForExistingDef(tok, varName);

      // XXX need to check for a default variable assignment
      
      ArgDefPtr argDef = context->builder.createArgDef(argType.get(), varName);
      args.push_back(argDef);
      addDef(argDef.get());
      
      // check for a comma
      tok = getToken();
      if (tok.isComma())
         tok = getToken();
      else if (!tok.isRParen())
         unexpected(tok, "expected ',' or ')' after argument definition");
   }
}

// oper init(...) : init1(expr), ... {
//                 ^                ^
void Parser::parseInitializers(Initializers *inits, Expr *receiver) {
   ContextPtr classCtx = context->getClassContext();
   TypeDefPtr type = TypeDefPtr::rcast(classCtx->ns);
   
   while (true) {
      // get an identifier
      Token tok = getToken();
      if (!tok.isIdent())
         unexpected(tok, "identifier expected in initializer list.");
      
      // try to look up an instance variable
      VarDefPtr varDef = context->ns->lookUp(tok.getData());
      if (!varDef || TypeDefPtr::rcast(varDef)) {
         // not a variable def, parse a type def.
         toker.putBack(tok);
         TypeDefPtr base =
            parseTypeSpec(" is neither a base class nor an instance variable");
            
         // try to find it in our base classes
         if (!type->isParent(base.get()))
            error(tok, 
                  SPUG_FSTR(base->getFullName() << 
                             " is not a direct base class of " <<
                             type->name
                            )
                  );
         
         // parse the arg list
         expectToken(Token::lparen, "expected an ergument list.");
         FuncCall::ExprVec args;
         parseMethodArgs(args);
         
         // look up the appropriate constructor
         FuncDefPtr operInit = context->lookUp("oper init", args, base.get());
         if (!operInit || operInit->getOwner() != base.get())
            error(tok, SPUG_FSTR("No matching constructor found for " <<
                                  base->getFullName()
                                 )
                  );
         
         FuncCallPtr funcCall = context->builder.createFuncCall(operInit.get());
         funcCall->args = args;
         funcCall->receiver = receiver;
         if (!inits->addBaseInitializer(base.get(), funcCall.get()))
            error(tok, 
                  SPUG_FSTR("Base class " << base->getFullName() <<
                             " already initialized."
                            )
                  );
      
      // make sure that it is a direct member of this class.
      } else if (varDef->getOwner() != type.get() &&
                 // it's likely that this is just a case of a parameter 
                 // shadowing an instance veriable, which is legal.  Try 
                 // looking up the variable at class scope.
                 (!(varDef = type->lookUp(tok.getData())) ||
                  varDef->getOwner() != type.get()
                  )
                 ) {
         error(tok,
               SPUG_FSTR(tok.getData() << " is not an immediate member of " <<
                        type->name
                        )
               );

      // make sure that it's an instance variable
      } else if (!varDef->hasInstSlot()) {
         error(tok,
               SPUG_FSTR(tok.getData() << " is not an instance variable.")
               );

      } else {
         // this is a normal, member initializer
   
         // this will be our initializer
         ExprPtr initializer;
   
         // get the next token
         Token tok2 = getToken();
         if (tok2.isLParen()) {
            // it's a left paren - treat this as a constructor.
            FuncCall::ExprVec args;
            parseMethodArgs(args);
            
            // look up the appropriate constructor
            FuncDefPtr operNew = 
               context->lookUp("oper new", args, varDef->type.get());
            if (!operNew)
               error(tok2,
                     SPUG_FSTR("No matching constructor found for instance "
                                "variable " << varDef->name <<
                                " of type " << varDef->type->name
                               )
                     );
            
            // construct a function call
            FuncCallPtr funcCall;
            initializer = funcCall =
               context->builder.createFuncCall(operNew.get());
            funcCall->args = args;
            
         } else if (tok2.isAssign()) {
            // it's the assignement operator, parse an expression
            initializer = parseExpression();
         } else {
            unexpected(tok2,
                       "expected constructor arg list or assignment operator"
                       );
         }
         
         // generate an assignment, add it to the initializers
         if (!inits->addFieldInitializer(varDef.get(), initializer.get()))
            error(tok,
                  SPUG_FSTR("Instance variable " << varDef->name <<
                            " already initialized."
                            )
                  );
      }
      
      // check for a comma
      tok = getToken();
      if (!tok.isComma()) {
         toker.putBack(tok);
         break;
      }
   }
}

int Parser::parseFuncDef(TypeDef *returnType, const Token &nameTok,
                         const string &name,
                         Parser::FuncFlags funcFlags,
                         int expectedArgCount
                         ) {
   runCallbacks(funcDef);

   // check for an existing, non-function definition.
   VarDefPtr existingDef = checkForExistingDef(nameTok, name, true);

   FuncDef::Flags nextFuncFlags = context->nextFuncFlags;

   // if this is a class context, we're defining a method.  We take the strict
   // definition of "in a class context," only functions immediately in a 
   // class context are methods of that class.
   ContextPtr classCtx;
   if (context->scope == Context::composite && context->parent &&
       context->parent->scope == Context::instance
       )
      classCtx = context->parent;
   bool isMethod = classCtx && (!nextFuncFlags || 
                                nextFuncFlags & FuncDef::method
                                );
   TypeDef *classTypeDef = 0;
   
   // push a new context, arg defs will be stored in the new context.
   ContextPtr subCtx = context->createSubContext(Context::local, 0,
                                                 &name
                                                 );
   ContextStackFrame cstack(*this, subCtx.get());
   context->returnType = returnType;
   context->toplevel = true;
   
   // if this is a method, add the "this" variable
   ExprPtr receiver;
   if (isMethod) {
      assert(classCtx && "method not in class context.");
      classTypeDef = TypeDefPtr::arcast(classCtx->ns);
      ArgDefPtr argDef = context->builder.createArgDef(classTypeDef, "this");
      addDef(argDef.get());
      receiver = context->createVarRef(argDef.get());
   }

   // parse the arguments
   FuncDef::ArgVec argDefs;
   parseArgDefs(argDefs, isMethod);
   
   // if we are expecting an argument definition, check for it.
   if (expectedArgCount > -1 && argDefs.size() != expectedArgCount)
      error(nameTok, 
            SPUG_FSTR("Expected " << expectedArgCount <<
                      " arguments for function " << name
                      )
            );
   
   // a function is virtual if a) it is a method, b) the class has a vtable 
   // and c) it is neither implicitly or explicitly final.
   bool isVirtual = isMethod && classTypeDef->hasVTable && 
                    !TypeDef::isImplicitFinal(name) &&
                    (!nextFuncFlags || nextFuncFlags & FuncDef::virtualized);

   // If we're overriding/implementing a previously declared virtual 
   // function, we'll store it here.
   FuncDefPtr override;

   // we now need to verify that the new definition doesn't hide an 
   // existing definition.
   OverloadDef *existingOvldDef = OverloadDefPtr::rcast(existingDef);
   if (existingOvldDef && 
      (override = existingOvldDef->getSigMatch(argDefs)) &&
      !override->isOverridable()
      )
      error(nameTok,
            SPUG_FSTR("Definition of " << name <<
                     " hides previous overload."
                     )
            );
   
   // if we're "overriding" a forward declaration, use the namespace of the 
   // forward declaration.
   if (override && override->flags & FuncDef::forward)
      context->ns = override->ns;
   
   // make sure that the return type is exactly the same as the override
   if (override && override->returnType != returnType)
      error(nameTok,
            SPUG_FSTR("Function return type of " << 
                       returnType->getFullName() <<
                       " does not match that of the function it overrides (" <<
                       override->returnType->getFullName() << ")"
                       )
            );

   // figure out what the flags are going to be.
   FuncDef::Flags flags =
      (isMethod ? FuncDef::method : FuncDef::noFlags) |
      (isVirtual ? FuncDef::virtualized : FuncDef::noFlags);
   
   Token tok3 = getToken();
   InitializersPtr inits;
   if (tok3.isSemi()) {
      // forward declaration or stub - see if we've got a stub 
      // definition
      toker.putBack(tok3);
      StubDef *stub;
      FuncDefPtr funcDef;
      if (existingDef && (stub = StubDefPtr::rcast(existingDef))) {
         funcDef =
            context->builder.createExternFunc(*context, FuncDef::noFlags,
                                              name,
                                              returnType,
                                              0,
                                              argDefs,
                                              stub->address,
                                              name.c_str()
                                              );
         stub->getOwner()->removeDef(stub);
         cstack.restore();
         addFuncDef(funcDef.get());
      } else if (override) {
         // forward declarations of overrides don't make any sense.
         warn(tok3, SPUG_FSTR("Unnecessary forward declaration for overriden "
                               "function " << name << " (defined in ancestor "
                               "class " << 
                               override->getOwner()->getNamespaceName() <<
                               ")"
                              )
              );
      } else {
         // it's a forward declaration
         funcDef = context->builder.createFuncForward(*context, 
                                                      FuncDef::forward | 
                                                       flags,
                                                      name,
                                                      returnType,
                                                      argDefs,
                                                      override.get()
                                                      );

         cstack.restore();
         addFuncDef(funcDef.get());

         // if this is a constructor, and the user hasn't introduced their own 
         // "oper new", generate one for the new constructor now.
         if (funcFlags & hasMemberInits && !classTypeDef->gotExplicitOperNew)
            classTypeDef->createNewFunc(*classCtx, funcDef.get());
      }
      return argDefs.size();
   } else if (funcFlags == hasMemberInits) {
      inits = new Initializers();
      if (tok3.isColon()) {
         parseInitializers(inits.get(), receiver.get());
         tok3 = getToken();
      }
   }

   if (!tok3.isLCurly()) {
      unexpected(tok3, "expected '{' in function definition");
   }
   
   // if we got a forward declaration, make sure the args and the context are 
   // the same
   if (override && override->flags & FuncDef::forward) {
      if(!override->matchesWithNames(argDefs))
         error(tok3, 
               SPUG_FSTR("Argument list of function " << name << 
                        " doesn't match the names of its forward "
                        "declaration:\n    forward: " << override->args <<
                        "\n    defined: " << argDefs
                        )
               );
      
      if (override->getOwner() != context->getParent()->getDefContext()->ns.get())
         error(tok3,
               SPUG_FSTR("Function " << name << 
                          " can not be defined in a different namespace from "
                          "its forward declaration."
                         )
               );
   }

   // parse the body
   FuncDefPtr funcDef =
      context->builder.emitBeginFunc(*context, flags, name, returnType,
                                     argDefs,
                                     override.get()
                                     );

   // store the new definition in the parent context if it's not already in 
   // there (if there was a forward declaration)
   if (!funcDef->getOwner()) {
      ContextStackFrame cstack(*this, context->getParent().get());
      addFuncDef(funcDef.get());
   }

   // if there were initializers, emit them.
   if (inits)
      classTypeDef->emitInitializers(*context, inits.get());
   
   // run begin function callbacks.
   runCallbacks(funcEnter);

   // if this is an "oper del" with base & member cleanups, store them in the 
   // current cleanup frame
   if (funcFlags == hasMemberDels) {
      assert(classCtx && "emitting a destructor outside of class context");
      classTypeDef->addDestructorCleanups(*context);
   }

   ContextPtr terminal = parseBlock(true, funcLeave);
   
   // if the block doesn't always terminate, either give an error or 
   // return void if the function return type is void
   if (!terminal)
      if (context->construct->voidType->matches(*context->returnType)) {
         // remove the cleanup stack - we have already done cleanups at 
         // the block level.
         context->cleanupFrame = 0;
         context->builder.emitReturn(*context, 0);
      } else {
         // XXX we don't have the closing curly brace location, 
         // currently reporting the error on the top brace
         error(tok3, "missing return statement for non-void function.");
      }

   context->builder.emitEndFunc(*context, funcDef.get());
   cstack.restore();

   // clear the next function flags (we're done with them)
   context->nextFuncFlags = FuncDef::noFlags;

   // if this is an init function, and the user hasn't introduced an explicit
   // "oper new", and we haven't already done this for a forward declaration,
   // generate the corresponding "oper new".
   if (inits && !classTypeDef->gotExplicitOperNew && 
       (!override || !(override->flags & FuncDef::forward))
       )
      classTypeDef->createNewFunc(*classCtx, funcDef.get());
   
   return argDefs.size();
}         
         

// type var = initializer, var2 ;
//     ^                         ^
// type function() { }
//     ^              ^
bool Parser::parseDef(TypeDef *type) {
   Token tok2 = getToken();
   
   // if we get a '[', parse the specializer and get a generic type.
   if (tok2.isLBracket()) {
      type = parseSpecializer(tok2, type);
      tok2 = getToken();
   } else if(type->generic) {
      error(tok2, SPUG_FSTR("Generic type " << type->name << 
                             " must be specialized to be used."
                            )
            );
   }
   
   while (true) {
      if (tok2.isIdent()) {
         string varName = tok2.getData();
   
         // this could be a variable or a function
         Token tok3 = getToken();
         if (tok3.isSemi() || tok3.isComma()) {
            // it's a variable.
            runCallbacks(variableDef);

            // make sure we're not hiding anything else
            checkForExistingDef(tok2, tok2.getData());
            
            // we should now _always_ have a default constructor.
            assert(type->defaultInitializer);
            
            // Emit a variable definition and store it in the context (in a 
            // cleanup frame so transient initializers get destroyed here)
            context->emitVarDef(type, tok2, 0);
            
            if (tok3.isSemi())
               return true;
            else {
               tok2 = getToken();
               continue;
            }
         } else if (tok3.isAssign()) {
            runCallbacks(variableDef);
            ExprPtr initializer;
   
            // make sure we're not hiding anything else
            checkForExistingDef(tok2, tok2.getData());
   
            // check for a curly brace, indicating construction args.
            Token tok4 = getToken();
            if (tok4.isLCurly()) {
               // got constructor args, parse an arg list terminated by a right 
               // curly.
               initializer = parseConstructor(tok4, type, Token::rcurly);
            } else if (tok4.isLBracket()) {
               initializer = parseConstSequence(type);
            } else {
               toker.putBack(tok4);
               initializer = parseExpression();
            }
   
            // make sure the initializer matches the declared type.
            initializer = initializer->convert(*context, type);
            if (!initializer)
               error(tok4, "Incorrect type for initializer.");

            context->emitVarDef(type, tok2, initializer.get());
   
            // if this is a comma, we need to go back and parse 
            // another definition for the type.
            tok4 = getToken();
            if (tok4.isComma()) {
               tok2 = getToken();
               continue;
            } else if (tok4.isSemi()) {
               return true;
            } else {
               unexpected(tok4, 
                          "Expected comma or semicolon after variable "
                           "definition."
                          );
            }
         } else if (tok3.isLParen()) {
            // function definition
            parseFuncDef(type, tok2, tok2.getData(), normal, -1);
            return true;
         } else {
            unexpected(tok3,
                     "expected variable initializer or function "
                     "definition."
                     );
         }
      } else if (tok2.isOper()) {
         // deal with an operator
         parsePostOper(type);
         return true;
      }

      // if we haven't "continued", were done.
      toker.putBack(tok2);
      return false;
   }
}

ContextPtr Parser::parseIfClause() {
   Token tok = getToken();
   ContextPtr terminal;
   ContextStackFrame cstack(*this, context->createSubContext().get());
   if (tok.isLCurly()) {
      return parseBlock(true, noCallbacks);
   } else {
      toker.putBack(tok);
      return parseStatement(false);
   }
}
   
ExprPtr Parser::parseCondExpr() {
   TypeDef *boolType = context->construct->boolType.get();
   ExprPtr cond = parseExpression()->convert(*context, boolType);
   if (!cond)
      error(getToken(),  "Condition is not boolean.");
   
   return cond;
}

// clause := expr ;   (';' can be replaced with EOF)
//        |  { block }
// if ( expr ) clause
//   ^               ^
// if ( expr ) clause else clause
//   ^                           ^
ContextPtr Parser::parseIfStmt() {
   Token tok = getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after if");
   
   ExprPtr cond = parseCondExpr();
   
   tok = getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected closing paren");
   
   BranchpointPtr pos = context->builder.emitIf(*context, cond.get());

   ContextPtr terminalIf = parseIfClause();
   ContextPtr terminalElse;

   // check for the "else"
   tok = getToken();
   if (tok.isElse()) {
      pos = context->builder.emitElse(*context, pos.get(), terminalIf);
      terminalElse = parseIfClause();
      context->builder.emitEndIf(*context, pos.get(), terminalElse);
   } else {
      toker.putBack(tok);
      context->builder.emitEndIf(*context, pos.get(), terminalIf);
   }

   // the if is terminal if both conditions are terminal.  The terminal 
   // context is the innermost of the two.
   if (terminalIf && terminalElse)
      if (terminalIf->encloses(*terminalElse))
         return terminalElse;
      else
         return terminalIf;
   else
      return 0;
}

// while ( expr ) stmt ; (';' can be replaced with EOF)
//      ^               ^
// while ( expr ) { ... }
//      ^                ^
void Parser::parseWhileStmt() {
   // create a subcontext for the break and for variables defined in the 
   // condition.
   ContextStackFrame cstack(*this, context->createSubContext().get());

   Token tok = getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after while");

   // parse the condition   
   ExprPtr cond = parseCondExpr();

   tok = getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected right paren after conditional expression");
   
   BranchpointPtr pos =
      context->builder.emitBeginWhile(*context, cond.get(), false);
   context->setBreak(pos.get());
   context->setContinue(pos.get());
   ContextPtr terminal = parseIfClause();
   context->builder.emitEndWhile(*context, pos.get(), terminal);
}

// for ( ... ) { ... }
//    ^               ^
// for ( ... ) stmt ; (';' can be replaced with EOF)
//    ^              ^
void Parser::parseForStmt() {
   // create a subcontext for the break and for variables defined in the 
   // condition.
   ContextStackFrame cstack(*this, context->createSubContext().get());

   Token tok = getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after for");

   // the condition, before and after body expressions.
   ExprPtr cond, beforeBody, afterBody;
   bool iterForm = false;
   
   // check for 'ident in', 'ident :in', 'ident on' or 'ident :on'
   tok = getToken();
   if (tok.isIdent()) {
      bool definesVar, varIsIter;
      Token tok2 = getToken();
      if (tok2.isIn()) {
         definesVar = false;
         varIsIter = false;
         iterForm = true;
      } else if (tok2.isColon()) {
         Token tok3 = getToken();
         definesVar = true;
         if (tok3.isIn()) {
            varIsIter = false;
            iterForm = true;
         } else if (tok3.isOn()) {
            varIsIter = true;
            iterForm = true;
         } else {
            toker.putBack(tok3);
         }
      } else if (tok2.isOn()) {
         definesVar = false;
         varIsIter = true;
         iterForm = true;
      }
      
      if (iterForm) {
         // got iteration - parse the expression that we're iterating over 
         // and expand the iteration expressions.

         // parse the list expression
         ExprPtr expr = parseExpression();

         // let the context expand an iteration expression            
         context->expandIteration(tok.getData(), definesVar, varIsIter,
                                  expr.get(), 
                                  cond, 
                                  beforeBody,
                                  afterBody
                                  );

         // check for a closing paren
         tok = getToken();
         if (!tok.isRParen())
            unexpected(tok, "expected closing parenthesis");
      } else {
         toker.putBack(tok2);
      }
   }
   
   // if we fall through to here, this is a C-like for statement
   if (!iterForm) {
      toker.putBack(tok);
   
      // parse the initialization clause (if any)
      tok = getToken();
      if (!tok.isSemi()) {
         toker.putBack(tok);
         parseClause(true);
      }
   
      // check for a conditional expression
      tok = getToken();
      if (tok.isSemi()) {
         // no conditional, create one from a constant
         TypeDef *boolType = context->construct->boolType.get();
         cond = context->builder.createIntConst(*context, 1)->convert(*context,
                                                                     boolType
                                                                     );
      } else {
         toker.putBack(tok);
         cond = parseCondExpr();
   
         tok = getToken();
         if (!tok.isSemi())
            unexpected(tok, "expected semicolon after condition");
         
      }
   
      // check for an after-body expression
      tok = getToken();
      if (!tok.isRParen()) {
         toker.putBack(tok);
   
         afterBody = parseExpression();
         tok = getToken();
         if (!tok.isRParen())
            unexpected(tok, "expected closing parenthesis");
      }
   }

   // emit the loop
   BranchpointPtr pos =
      context->builder.emitBeginWhile(*context, cond.get(), afterBody);
   context->setBreak(pos.get());
   context->setContinue(pos.get());
   
   // emit the before-body expression if there was one
   if (beforeBody) {
      context->createCleanupFrame();
      beforeBody->emit(*context)->handleTransient(*context);
      context->closeCleanupFrame();
   }
   
   // parse the loop body
   ContextPtr terminal = parseIfClause();
   
   // emit the after-body expression if there was one.
   if (afterBody) {
      context->builder.emitPostLoop(*context, pos.get(), terminal);
      context->createCleanupFrame();
      afterBody->emit(*context)->handleTransient(*context);
      context->closeCleanupFrame();
      terminal = false;
   }

   context->builder.emitEndWhile(*context, pos.get(), terminal);
   
   // close any variables created for the loop context.
   context->builder.closeAllCleanups(*context);
}

void Parser::parseReturnStmt() {
   // check for a return with no expression
   Token tok = getToken();
   bool returnVoid = false;
   if (tok.isSemi()) {
      returnVoid = true;
   } else if (tok.isEnd() || tok.isRCurly()) {
      toker.putBack(tok);
      returnVoid = true;
   }
   if (returnVoid) {
      if (!context->returnType->matches(*context->construct->voidType))
         error(tok,
               SPUG_FSTR("Missing return expression for function "
                          "returning " << context->returnType->name
                         )
               );
      context->builder.emitReturn(*context, 0);
      return;
   }
   // if return type is void, but they are trying to return an expression,
   // fail with message
   else if (context->returnType == context->construct->voidType) {
      error(tok,
            SPUG_FSTR("Cannot return expression from function "
                      "with return type void")
            );
   }

   // parse the return expression, make sure that it matches the return type.
   toker.putBack(tok);
   ExprPtr orgExpr = parseExpression();
   ExprPtr expr = orgExpr->convert(*context, context->returnType.get());
   if (!expr)
      error(tok,
            SPUG_FSTR("Invalid return type " << orgExpr->type->name <<
                       " for function returning " << context->returnType->name
                      )
            );
   else if (!expr && 
            !context->construct->voidType->matches(*context->returnType))
      error(tok,
            SPUG_FSTR("Missing return value for function returning " <<
                       context->returnType->name
                      )
            );
   
   // emit the return statement
   context->builder.emitReturn(*context, expr.get());

   tok = getToken();   
   if (tok.isEnd() || tok.isRCurly())
      toker.putBack(tok);
   else if (!tok.isSemi())
      unexpected(tok, "expected semicolon or block terminator");
}

// import module-and-defs ;
//       ^               ^
void Parser::parseImportStmt(Namespace *ns) {
   ModuleDefPtr mod;
   string canonicalName;
   builder::Builder &builder = context->construct->getCurBuilder();

   Token tok = getToken();
   if (tok.isIdent()) {
      toker.putBack(tok);
      vector<string> moduleName;
      parseModuleName(moduleName);
            
      mod = context->construct->loadModule(moduleName.begin(), 
                                           moduleName.end(),
                                           canonicalName
                                           );
      if (!mod)
         error(tok, SPUG_FSTR("unable to find module " << canonicalName));
      
      // make sure the module is finished (no recursive imports)
      else if (!mod->finished)
         error(tok,
               SPUG_FSTR("Attempting to import module " << canonicalName <<
                          " recursively."
                         )
               );
      else
         builder.initializeImport(mod.get(),
                                  // HACK check for annotation?
                                  ns == context->compileNS.get()
                                  );

   } else if (!tok.isString()) {
      unexpected(tok, "expected string constant");
   }
   
   string name = tok.getData();
   
   // parse all following symbols
   vector<string> syms;
   while (true) {
      tok = getToken();
      if (tok.isIdent()) {
         syms.push_back(tok.getData());
         tok = getToken();
         if (tok.isSemi()) {
            break;
         } else if (!tok.isComma()) {
            unexpected(tok, "expected comma or semicolon");
         }
      } else if (tok.isSemi()) {
         break;
      } else {
         unexpected(tok, "expected identifier or semicolon");
      }
   }

   if (!mod) {
      try {
         builder.importSharedLibrary(name, syms, *context, ns);
      } catch (const spug::Exception &ex) {
         error(tok, ex.getMessage());
      }
   } else {
      // alias all of the names in the new module
      for (vector<string>::iterator iter = syms.begin();
           iter != syms.end();
           ++iter
           ) {
         // make sure we don't already have it
         if (ns->lookUp(*iter))
            error(tok, SPUG_FSTR("imported name " << *iter << 
                                  " hides existing definition."
                                 )
                  );
         VarDefPtr symVal = mod->lookUp(*iter);
         if (!symVal)
            error(tok, SPUG_FSTR("name " << *iter << 
                                  " is not defined in module " << 
                                  canonicalName
                                 )
                  );
         builder.registerImportedDef(*context, symVal.get());
         ns->addAlias(symVal.get());
      }
   }
}

// try { ... } catch (...) { ... }
//    ^                           ^
ContextPtr Parser::parseTryStmt() {
   Token tok = toker.getToken();
   if (!tok.isLCurly())
      unexpected(tok, "Curly bracket expected after try.");
   
   BranchpointPtr pos = context->builder.emitBeginTry(*context);

   // create a subcontext for the try statement
   ContextStackFrame cstack(*this, context->createSubContext().get());
   context->setCatchBranchpoint(pos.get());

   ContextPtr terminal;
   {
      ContextStackFrame cstack(*this, context->createSubContext().get());
      terminal = parseBlock(true, noCallbacks); // XXX add tryLeave callback
   }
   bool lastWasTerminal = terminal;
   
   // strip the catch branchpoint so that exceptions thrown in the 
   // finally/catch clauses are thrown to outer contexts. 
   context->setCatchBranchpoint(0);
   
   tok = toker.getToken();
   if (!tok.isCatch())
      unexpected(tok, "catch expected after try block.");
   
   while (true) {
      
      // parse the exception specifier
      tok = toker.getToken();
      if (!tok.isLParen())
         unexpected(tok, 
                    "parenthesized catch expression expected after catch "
                     "keyword."
                    );

      TypeDefPtr exceptionType = parseTypeSpec();
      
      // parse the exception variable
      Token varTok = toker.getToken();
      if (!varTok.isIdent())
         unexpected(tok, "variable name expected after exception type.");

      ExprPtr exceptionObj =
         context->builder.emitCatch(*context, pos.get(), exceptionType.get(),
                                    lastWasTerminal
                                    );
      
      tok = toker.getToken();
      if (!tok.isRParen())
         unexpected(tok, 
                    "closing parenthesis expected after exception variable."
                    );
      
      // parse the catch body
      tok = toker.getToken();
      if (!tok.isLCurly())
         unexpected(tok,
                    "Curly bracket expected after catch clause."
                    );
      
      {
         ContextStackFrame cstack(*this, context->createSubContext().get());
         
         // create a variable definition for the exception variable
         context->emitVarDef(exceptionType.get(), varTok, exceptionObj.get());
         
         // give the builder an opportunity to add an exception cleanup
         context->builder.emitExceptionCleanup(*context);
         
         // XXX add catchLeave callback
         ContextPtr terminalCatch = parseBlock(true, noCallbacks); 
         lastWasTerminal = terminalCatch;
         if (terminalCatch) {
            if (terminal && terminal->encloses(*terminalCatch))
               // need to replace the terminal context to the closer terminal 
               // context for the catch
               terminal = terminalCatch;
         } else {
            // non-terminal catch, therefore the entire try/catch statement 
            // is not terminal.
            terminal = 0;
         }
      }
      
      // see if there's another catch
      tok = toker.getToken();
      if (!tok.isCatch()) {
         toker.putBack(tok);
         context->builder.emitEndTry(*context, pos.get(), lastWasTerminal);
         return terminal;
      }
   }
}

ContextPtr Parser::parseThrowStmt() {
   Token tok = toker.getToken();
   if (tok.isSemi()) {
      // XXX need to get this working and to verify that we are in a catch
      error(tok, "Rethrowing exceptions not supported yet.");
//      context->builder.emitThrow(*context, 0);
   } else {
      toker.putBack(tok);
      ExprPtr expr = parseExpression();

      if (!expr->type->isDerivedFrom(context->construct->vtableBaseType.get()))
         error(tok, SPUG_FSTR("Object of type " << expr->type->getFullName() <<
                               " is not derived from VTableBase."
                              )
               );
      
      tok = toker.getToken();
      if (!tok.isSemi())
         unexpected(tok, "Semicolon expected after throw expression.");
      
      context->builder.emitThrow(*context, expr.get());
   }

   // get the terminal context - if it's a toplevel context, we actually want 
   // to go one step further up.
   ContextPtr terminal = context->getCatch();
   if (terminal->toplevel)
      terminal = terminal->parent;
   return terminal;
}

// oper name ( args ) { ... }
//     ^                     ^
void Parser::parsePostOper(TypeDef *returnType) {

   Token tok = getToken();
   if (tok.isIdent()) {
      const string &ident = tok.getData();
      bool isInit = ident == "init";
      if (isInit || ident == "release" || ident == "bind" || ident == "del") {
         
         // these can only be defined in an instance context
         if (context->scope != Context::composite)
            error(tok, 
                  SPUG_FSTR("oper " << ident << 
                             " can only be defined in a class scope."
                            )
                  );
         
         // these opers must be of type "void"
         if (!returnType)
            context->returnType = returnType =
               context->construct->voidType.get();
         else if (returnType != context->construct->voidType.get())
            error(tok, 
                  SPUG_FSTR("oper " << ident << 
                            " must be of return type 'void'"
                            )
                  );
         expectToken(Token::lparen, "expected argument list");
         
         // the operators other than "init" require an empty args list.
         int expectedArgCount;
         if (!isInit)
            expectedArgCount = 0;
         else
            expectedArgCount = -1;

         FuncFlags flags;
         if (isInit)
            flags = hasMemberInits;
         else if (ident == "del")
            flags = hasMemberDels;
         else
            flags = normal;

         parseFuncDef(returnType, tok, "oper " + ident, flags, 
                      expectedArgCount
                      );
      } else if (ident == "x") {
         // "oper x++" or "oper x--"
         if (!returnType)
            error(tok, "operator requires a return type");
         tok = getToken();
         if (tok.isIncr() || tok.isDecr()) {
            expectToken(Token::lparen, "expected argument list.");         
            parseFuncDef(returnType, tok, "oper x" + tok.getData(),
                         normal,
                         (context->scope == Context::composite) ? 0 : 1
                         );
         } else {
            error(tok, "++ or -- expected after 'oper x' definition");
         }
      } else {
         unexpected(tok, "only 'oper init' honored at this time");
      }

   } else {
      
      // all others require a return type
      if (!returnType)
         error(tok, "operator requires a return type");

      if (tok.isLBracket()) {
         // "oper []" or "oper []="
         expectToken(Token::rbracket, "expected right bracket.");
         tok = getToken();
         if (context->scope != Context::composite)
            error(tok, 
                  "Bracket operators may only be defined in class scope."
                  );
         if (tok.isAssign()) {
            expectToken(Token::lparen, "expected argument list.");
            parseFuncDef(returnType, tok, "oper []=", normal, 2);
         } else {
            parseFuncDef(returnType, tok, "oper []", normal, 1);
         }
      } else if (tok.isMinus()) {
         // minus is special because it can be either unary or binary
         expectToken(Token::lparen, "expected argument list.");
         int numArgs = parseFuncDef(returnType, tok, "oper " + tok.getData(), 
                                    normal, 
                                    -1
                                    );
         
         int receiverCount = (context->scope == Context::composite);
         if (numArgs != 1 - receiverCount && numArgs != 2 - receiverCount)
            error(tok, SPUG_FSTR("'oper -' must have " << 1 - receiverCount <<
                                 " or " << 2 - receiverCount <<
                                 " arguments."
                                 )
                  );
      } else if (tok.isTilde() || tok.isBang()) {
         expectToken(Token::lparen, "expected an argument list.");
         // in composite context, these should have no arguments.
         int numArgs = (context->scope == Context::composite) ? 0 : 1;
         parseFuncDef(returnType, tok, "oper " + tok.getData(), normal, 
                      numArgs
                      );
      } else if (tok.isIncr() || tok.isDecr()) {
         string sym = tok.getData();
         tok = getToken();
         if (!tok.isIdent() || tok.getData() != "x")
            unexpected(tok, 
                       "increment/decrement operators must include an 'x' "
                        "token to indicate pre or post: ex: oper ++x()"
                       );
         expectToken(Token::lparen, "expected argument list.");
         parseFuncDef(returnType, tok, "oper " + sym + "x",
                      normal,
                      (context->scope == Context::composite) ? 0 : 1
                      );
      } else if (tok.isBinOp() || tok.isAugAssign()) {
         // binary operators
         
         // in composite context, these should have just one argument.
         int numArgs = (context->scope == Context::composite) ? 1 : 2;
         
         expectToken(Token::lparen, "expected argument list.");
         parseFuncDef(returnType, tok, "oper " + tok.getData(), normal, 
                      numArgs
                      );
      } else {
         unexpected(tok, "identifier or symbol expected after 'oper' keyword");
      }
   }
}

// class name : base, base { ... }
//      ^                         ^
// class name;
//      ^     ^
TypeDefPtr Parser::parseClassDef() {
   runCallbacks(classDef);

   Token tok = getToken();
   if (!tok.isIdent())
      unexpected(tok, "Expected class name");
   string className = tok.getData();
   
   // check for an existing definition of the symbol
   TypeDefPtr existing = checkForExistingDef(tok, tok.getData());
   
   // parse base class list   
   vector<TypeDefPtr> bases;
   vector<TypeDefPtr> ancestors;  // keeps track of all ancestors
   tok = getToken();
   if (tok.isColon())
      while (true) {
         // parse the base class name, make sure it's safe to add this as a 
         // base class given the existing set, and add it to the list.
         TypeDefPtr baseClass = parseTypeSpec();
         baseClass->addToAncestors(*context, ancestors);
         bases.push_back(baseClass);
         
         tok = getToken();
         if (tok.isLCurly())
            break;
         else if (!tok.isComma())
            unexpected(tok, "expected comma or opening brace");
      }
   else if (tok.isSemi())
      // forward declaration
      return context->createForwardClass(className);
   else if (!tok.isLCurly())
      unexpected(tok, "expected colon or opening brace.");
   
   // if no base classes were specified, and Object has been defined, make 
   // Object the implicit base class.
   if (!bases.size() && context->construct->objectType)
      bases.push_back(context->construct->objectType);

   // create a class context
   ContextPtr classContext =
      new Context(context->builder, Context::instance, context.get(), 0, 
                  context->compileNS.get()
                  );

   // emit the beginning of the class, hook it up to the class context and 
   // store a reference to it in the parent context.
   TypeDefPtr type =
      context->builder.emitBeginClass(*classContext, className, bases,
                                      existing.get()
                                      );
   if (!existing)
      addDef(type.get());
   
   // add the "cast" method
   if (type->hasVTable)
      type->createCast(*classContext);

   // create a lexical context which delegates to both the class context and 
   // the parent context.
   NamespacePtr lexicalNS =
      new CompositeNamespace(type.get(), context->ns.get());
   ContextPtr lexicalContext = 
      classContext->createSubContext(Context::composite, lexicalNS.get());

   // push the new context
   ContextStackFrame cstack(*this, lexicalContext.get());

   // parse the body
   parseClassBody();

   type->rectify(*classContext);
   type->complete = true;
   classContext->builder.emitEndClass(*classContext);
   cstack.restore();
   
   return type;
}

Parser::Parser(Toker &toker, model::Context *context) : 
   toker(toker),
   moduleCtx(context),
   context(context) {
   
   // build the precedence table
   enum {  noPrec, logOrPrec, logAndPrec, bitOrPrec, bitXorPrec, bitAndPrec, 
           cmpPrec, shiftPrec, addPrec, multPrec, unaryPrec
         };
   struct { const char *op; unsigned prec; } map[] = {
      
      // unary operators are distinguished from their non-unary forms by 
      // appending an "x"
      {"!x", unaryPrec},
      {"-x", unaryPrec},
      {"--x", unaryPrec},
      {"++x", unaryPrec},
      {"~x", unaryPrec},

      {"*", multPrec},
      {"/", multPrec},
      {"%", multPrec},
      {"+", addPrec},
      {"<<", shiftPrec},
      {">>", shiftPrec},
      {"-", addPrec},
      {"==", cmpPrec},
      {"!=", cmpPrec},
      {"<", cmpPrec},
      {">", cmpPrec},
      {"<=", cmpPrec},
      {">=", cmpPrec},
      {"is", cmpPrec},
      {"&", bitAndPrec},
      {"^", bitXorPrec},
      {"|", bitOrPrec},
      {"&&", logAndPrec},
      {"||", logOrPrec},
      
      {0, noPrec}
   };
   
   for (int i = 0; map[i].op; ++i)
      opPrecMap[map[i].op] = map[i].prec;
}   

void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false, noCallbacks);
}

// class name { ... }
//             ^     ^
void Parser::parseClassBody() {
   runCallbacks(classEnter);

   // parse the class body   
   while (true) {
      state = st_base;
      
      // check for a closing brace or a nested class definition
      Token tok = getToken();
      if (tok.isRCurly()) {
         // run callbacks, this can change the token stream so make sure we've 
         // still got an end curly
         toker.putBack(tok);
         if (runCallbacks(classLeave)) {
            Token tok2 = toker.getToken();
            if (!tok2.isRCurly()) {
               toker.putBack(tok2);
               continue;
            }
         } else {
            toker.getToken();
         }
         break;
      } else if (tok.isSemi()) {
         // ignore stray semicolons
         continue;
      } else if (tok.isClass()) {
         state = st_notBase;
         TypeDefPtr newType = parseClassDef();
         continue;

      // check for "oper" keyword
      } else if (tok.isOper()) {
         state = st_notBase;
         parsePostOper(0);
         continue;
      }
      
      // parse some other kind of definition
      toker.putBack(tok);
      state = st_notBase;
      TypeDefPtr type = parseTypeSpec();
      parseDef(type.get());
   }
   
   // make sure all forward declarations have been defined.
   context->parent->checkForUnresolvedForwards();
}

VarDefPtr Parser::checkForExistingDef(const Token &tok, const string &name, 
                                      bool overloadOk
                                      ) {
   ContextPtr classContext;
   VarDefPtr existing = context->lookUp(name);
   if (existing) {
      Namespace *existingNS = existing->getOwner();
      TypeDef *existingClass = 0;

      // if it's ok to overload, make sure that the existing definition is a 
      // function or an overload def or a stub.
      if (overloadOk && (FuncDefPtr::rcast(existing) || 
                         OverloadDefPtr::rcast(existing) ||
                         StubDefPtr::rcast(existing)
                         )
          )
         return existing;

      // check for forward declarations
      if (existingNS == context->ns.get()) {
         
         // forward function
         FuncDef *funcDef;
         if ((funcDef = FuncDefPtr::rcast(existing)) &&
             funcDef->flags & FuncDef::forward
             )
            // treat a forward declaration the same as an overload.
            return existing;
      
         // forward class
         TypeDef *typeDef;
         if ((typeDef = TypeDefPtr::rcast(existing)) &&
             typeDef->forward
             )
            return existing;

         // redefinition in the same context is otherwise an error
         redefineError(tok, existing.get());
      }      
   }
   
   return 0;
}

void Parser::redefineError(const Token &tok, const VarDef *existing) {
   error(tok, 
         SPUG_FSTR("Symbol " << existing->name <<
                    " is already defined in this context."
                   )
         );
}

void Parser::error(const Token &tok, const std::string &msg) {
   context->error(tok.getLocation(), msg);
}

void Parser::warn(const Location &loc, const std::string &msg) {
   context->warn(loc, msg);
}

void Parser::warn(const Token &tok, const std::string &msg) {
   warn(tok.getLocation(), msg);
}

void Parser::addCallback(Parser::Event event, ParserCallback *callback) {
   assert(event < eventSentinel);
   callbacks[event].push_back(callback);
}

bool Parser::removeCallback(Parser::Event event, ParserCallback *callback) {
   assert(event < eventSentinel);
   CallbackVec &cbs = callbacks[event];
   for (CallbackVec::iterator iter = cbs.begin(); iter != cbs.end(); ++iter)
      if (*iter == callback) {
         cbs.erase(iter);
         return true;
      }
   
   // callback was not found
   return false;
}

bool Parser::runCallbacks(Event event) {
   assert(event < eventSentinel);
   CallbackVec &cbs = callbacks[event];
   bool gotCallbacks = cbs.size();
   for (int i = 0; i < cbs.size(); ++i)
      cbs[i]->run(this, &toker, context.get());
   return gotCallbacks;
}
