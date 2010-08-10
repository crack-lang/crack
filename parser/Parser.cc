// Copyright 2009 Google Inc.

#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include <spug/Exception.h>
#include <spug/StringFmt.h>
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
#include "Crack.h"
#include "ParseError.h"
#include <cstdlib>

using namespace std;
using namespace parser;
using namespace model;

void Parser::addDef(VarDef *varDef) {
   ContextPtr defContext = context->getDefContext();
   defContext->ns->addDef(varDef);
   
   // if the definition context is a class context and the definition is a 
   // function and this isn't the "Class" class (which is its own meta-class), 
   // add it to the meta-class.
   FuncDef *func;
   TypeDef *type;
   if (defContext->scope == Context::instance &&
       (func = FuncDefPtr::cast(varDef))
       ) {
      type = TypeDefPtr::arcast(defContext->ns);
      if (type != type->type.get())
         type->type->addAlias(varDef);
   }
}

Token Parser::getToken() {
   Token tok = toker.getToken();
   context->setLocation(tok.getLocation());
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

void Parser::parseClause(bool defsAllowed) {
   Token tok = getToken();
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
            if (def->owner == context->ns.get())
               redefineError(tok2, def.get());
            else
               warn(tok2, SPUG_FSTR("Redefining symbol " << def->name));
            
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
         expr = parseExpression();
      }

   } else {
      toker.putBack(tok);
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

ContextPtr Parser::parseStatement(bool defsAllowed) {
   // peek at the next token
   Token tok = getToken();

   // check for statements
   if (tok.isSemi()) {
      // null statement
      return 0;
   } else if (tok.isIf()) {
      return parseIfStmt();
   } else if (tok.isWhile()) {
      parseWhileStmt();
      
      // while statements are never terminal, there's always the possibility 
      // that we could never execute the body.
      return 0;
   } else if (tok.isElse()) {
      unexpected(tok, "'else' with no matching 'if'");
   } else if (tok.isReturn()) {
      if (context->scope == Context::module)
         error(tok, "Return statement not allowed in module scope");
      parseReturnStmt();
      return context->getToplevel()->getParent();
   } else if (tok.isImport()) {
      parseImportStmt();
      return 0;
   } else if (tok.isClass()) {
      if (!defsAllowed)
         error(tok, "class definitions are not allowed in this context");
      parseClassDef();
      return 0;      
   } else if (tok.isBreak()) {
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
   }

   toker.putBack(tok);
   parseClause(defsAllowed);

   return 0;
}

ContextPtr Parser::parseBlock(bool nested) {
   Token tok;
   ContextPtr terminal;

   // keeps track of whether we've emitted a warning about stuff after a 
   // terminal statement.
   bool gotStuffAfterTerminalStatement = false;

   while (true) {

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

ExprPtr Parser::makeThisRef(const Token &ident, const string &memberName) {
   VarDefPtr thisVar = context->ns->lookUp("this");
   if (!thisVar)
      error(ident,
            SPUG_FSTR("instance member " << ident.getData() <<
                       " may not be used in a static context."
                      )
            );
                      
   return context->createVarRef(thisVar.get());
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

   // if the definition is for an instance variable, emit an implicit 
   // "this" dereference.  Otherwise just emit the variable
   if (TypeDefPtr::cast(var->owner)) {
      // if there's no container, try to use an implicit "this"
      ExprPtr receiver = container ? container : 
                                     makeThisRef(ident, ident.getData());
      return context->createFieldRef(receiver.get(), var.get());
   } else {
      return context->createVarRef(var.get());
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
       ident == "del"
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
   // XXX needs to handle callable objects.
   FuncDefPtr func = ns->lookUp(*context, funcName, args);
   if (!func)
      error(ident,
            SPUG_FSTR("No method exists matching " << funcName <<
                       " with these argument types."
                      )
            );

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
         if (container->type->meta && !TypeDefPtr::acast(func->owner)->meta) {
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
         receiver = makeThisRef(ident, funcName);
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
   
   // is it an assignment?
   Token tok1 = getToken();
   if (tok1.isAssign()) {
      
      if (ident.isOper())
         error(tok1, "Expected operator identifier after 'oper' keyword");

      VarDefPtr var = ns->lookUp(ident.getData());
      if (!var)
         error(tok1,
               SPUG_FSTR("attempted to assign undefined variable " <<
                          ident.getData()
                         )
               );
      
      // XXX need to verify that it's not a constant (or at least not a 
      // function)

      // parse an expression
      ExprPtr val = parseExpression();
      if (!val) {
         tok1 = getToken();
         error(tok1, "expression expected");
      }

      // if this is an instance variable, emit a field assignment.  
      // Otherwise emit a normal variable assignment.
      if (TypeDefPtr::cast(var->owner)) {
         // if there's no container, try to use an implicit "this"
         ExprPtr receiver = container ? container : 
                                        makeThisRef(ident, ident.getData());
         return AssignExpr::create(*context, ident, receiver.get(), var.get(), 
                                   val.get()
                                   );
      } else {
         return AssignExpr::create(*context, ident, var.get(), val.get());
      }
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
   TypeDef *boolType = context->globalData->boolType.get();
   ExprPtr cond = result->convert(*context, boolType);
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
      FuncDefPtr func = expr->type->lookUp(*context, "format", args);
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

TypeDef *Parser::convertTypeRef(Expr *expr) {
   VarRef *ref = VarRefPtr::cast(expr);
   if (!ref)
      return 0;
   
   return TypeDefPtr::rcast(ref->def);
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
               expr->type->lookUpNoArgs("oper class");
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
               expr->type->lookUp(*context, "oper []=", args);
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
               expr->type->lookUp(*context, "oper []", args);
            if (!funcDef)
               error(tok, 
                     SPUG_FSTR("'oper []' not defined for " <<
                               expr->type->name  << " with these arguments."
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
         FuncDefPtr funcDef = expr->type->lookUp(*context, symbol, args);
         if (!funcDef) {
            args.push_back(expr);
            funcDef = context->ns->lookUp(*context, symbol, args);
         }
         if (!funcDef)
            error(tok, SPUG_FSTR(symbol << " is not defined for this type."));
   
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
         
         FuncCall::ExprVec exprs(1);
         exprs[0] = rhs;
         std::string name = "oper " + tok.getData();
         
         // first try to find it in the type's context, then try to find it in 
         // the current context.
         FuncDefPtr func = expr->type->lookUp(*context, name, exprs);
         if (!func) {
            exprs[0] = expr;
            exprs.push_back(rhs);
            func = context->ns->lookUp(*context, name, exprs);
         }
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

ExprPtr Parser::parseExpression(unsigned precedence) {

   ExprPtr expr;

   // check for null
   Token tok = getToken();
   if (tok.isNull()) {
      expr = new NullConst(context->globalData->voidPtrType.get());
   
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
      expr = context->getStrConst(tok.getData());
   
   // for an interpolated string
   } else if (tok.isIstrBegin()) {
      assert(false && "istring expressions not yet implemented");
      // XXX we need to create a StringFormatter and pass it to parseIString() 
      // as the formatter.
//      expr = parseIString(formatter);
   
   // for an integer constant
   } else if (tok.isInteger()) {
      expr = context->builder.createIntConst(*context, 
                                             atoi(tok.getData().c_str())
                                             );
   } else if (tok.isFloat()) {
      expr = context->builder.createFloatConst(*context,
                                               atof(tok.getData().c_str())
                                               );

   } else if (tok.isPlus()) {
       // eat + if expression is a numeric constant and fail if it's not
       tok = getToken();
       if (tok.isInteger())
           expr = context->builder.createIntConst(*context,
                                                  atoi(tok.getData().c_str())
                                                  );
       else if(tok.isFloat())
           expr = context->builder.createFloatConst(*context,
                                                    atof(tok.getData().c_str())
                                                    );
       else
           unexpected(tok, "unexpected unary +");
   // for the unary operators
   } else if (tok.isBang() || tok.isMinus() || tok.isTilde() ||
              tok.isDecr() || tok.isIncr()) {
      FuncCall::ExprVec args;
      string symbol = tok.getData();

      // try to look it up for the expression, then for the context.
      ExprPtr operand = parseExpression(getPrecedence(symbol + "x"));
      symbol = "oper " + symbol;
      if (tok.isIncr() || tok.isDecr())
         symbol += "x";
      FuncDefPtr funcDef = operand->type->lookUp(*context, symbol, args);
      if (!funcDef) {
         args.push_back(operand);
         funcDef = context->ns->lookUp(*context, symbol, args);
      }
      if (!funcDef)
         error(tok, SPUG_FSTR(symbol << " is not defined for this type."));

      FuncCallPtr funcCall = context->builder.createFuncCall(funcDef.get());
      funcCall->args = args;
      if (funcDef->flags & FuncDef::method)
         funcCall->receiver = operand;
      expr = funcCall;
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
   FuncDefPtr func = type->lookUp(*context, "oper new", args);
   if (!func)
      error(tok,
            SPUG_FSTR("No constructor for " << type->name <<
                       " with these argument types."
                      )
            );
   
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
         FuncDefPtr operInit = 
            base->lookUp(*context, "oper init", args);
         if (!operInit || operInit->owner != base.get())
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
      } else if (varDef->owner != type.get()) {
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
               varDef->type->lookUp(*context, "oper new", args);
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
   // check for an existing, non-function definition.
   VarDefPtr existingDef = checkForExistingDef(nameTok, name, true);

   // if this is a class context, we're defining a method.  We take the strict
   // definition of "in a class context," only functions immediately in a 
   // class context are methods of that class.
   ContextPtr classCtx;
   if (context->scope == Context::composite && context->parent &&
       context->parent->scope == Context::instance
       )
      classCtx = context->parent;
   bool isMethod = classCtx ? true : false;
   TypeDef *classTypeDef = 0;

   // push a new context, arg defs will be stored in the new context.
   ContextPtr subCtx = context->createSubContext(Context::local);
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
   
   Token tok3 = getToken();
   InitializersPtr inits;
   if (tok3.isSemi()) {
      // abstract or forward declaration - see if we've got a stub 
      // definition
      toker.putBack(tok3);
      StubDef *stub;
      if (existingDef && (stub = StubDefPtr::rcast(existingDef))) {
         FuncDefPtr funcDef =
            context->builder.createExternFunc(*context, FuncDef::noFlags,
                                              name,
                                              returnType,
                                              argDefs,
                                              stub->address
                                              );
         stub->owner->removeDef(stub);
         cstack.restore();
         addDef(funcDef.get());
         return argDefs.size();
      } else {
         // XXX forward declaration
         error(tok3, 
               "abstract/forward declarations are not supported yet");
      }
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

   bool isVirtual = isMethod && classTypeDef->hasVTable && 
                    !TypeDef::isImplicitFinal(name);

   // If we're overriding/implementing a previously declared virtual 
   // function, we'll store it here.
   FuncDefPtr override;

   // XXX need to consolidate FuncDef and OverloadDef
   // we now need to verify that the new definition doesn't hide an 
   // existing definition.
   FuncDef *existingFuncDef = FuncDefPtr::rcast(existingDef);
   if (existingFuncDef && existingFuncDef->matches(argDefs)) {
      if (!(context->getDefContext()->ns.get() == existingDef->owner) ||
          !existingFuncDef->isOverridable()
          )
         override = existingFuncDef;
      else
         error(nameTok,
               SPUG_FSTR("Definition of " << name <<
                        "hides previous overload."
                        )
               );
   } else {
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

   }
   
   // parse the body
   FuncDef::Flags flags =
      (isMethod ? FuncDef::method : FuncDef::noFlags) |
      (isVirtual ? FuncDef::virtualized : FuncDef::noFlags);
   FuncDefPtr funcDef =
      context->builder.emitBeginFunc(*context, flags, name, returnType,
                                     argDefs,
                                     override.get()
                                     );

   // store the new definition in the parent context.
   {
      ContextStackFrame cstack(*this, context->getParent().get());
      addDef(funcDef.get());
   }

   // if there were initializers, emit them.
   if (inits)
      classTypeDef->emitInitializers(*context, inits.get());
   
   // if this is an "oper del" with base & member cleanups, store them in the 
   // current cleanup frame
   if (funcFlags == hasMemberDels) {
      assert(classCtx && "emitting a destructor outside of class context");
      classTypeDef->addDestructorCleanups(*context);
   }

   ContextPtr terminal = parseBlock(true);
   
   // if the block doesn't always terminate, either give an error or 
   // return void if the function return type is void
   if (!terminal)
      if (context->globalData->voidType->matches(*context->returnType)) {
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

   // if this is an init function, and the user hasn't introduced an explicit
   // "oper new", generate the corresponding "oper new".
   if (inits && !classTypeDef->gotExplicitOperNew)
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
   }
   
   while (true) {
      if (tok2.isIdent()) {
         string varName = tok2.getData();
   
         // this could be a variable or a function
         Token tok3 = getToken();
         if (tok3.isSemi() || tok3.isComma()) {
            // it's a variable.

            // make sure we're not hiding anything else
            checkForExistingDef(tok2, tok2.getData());
            
            // make sure we've got a default initializer
            if (!type->defaultInitializer)
               error(tok2, "no default constructor");
            
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
            ExprPtr initializer;
   
            // make sure we're not hiding anything else
            checkForExistingDef(tok2, tok2.getData());
   
            // check for a curly brace, indicating construction args.
            Token tok4 = getToken();
            if (tok4.isLCurly()) {
               // got constructor args, parse an arg list terminated by a right 
               // curly.
               initializer = parseConstructor(tok4, type, Token::rcurly);
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
   if (tok.isLCurly()) {
      ContextStackFrame cstack(*this, context->createSubContext().get());
      ContextPtr terminal = parseBlock(true);
      cstack.restore();
      return terminal;
   } else {
      toker.putBack(tok);
      return parseStatement(false);
   }
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
   
   TypeDef *boolType = context->globalData->boolType.get();
   ExprPtr cond = parseExpression()->convert(*context, boolType);
   if (!cond)
      error(tok, "Condition is not boolean.");
   
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
   
   ExprPtr expr = parseExpression();
   tok = getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected right paren after conditional expression");
   
   BranchpointPtr pos = context->builder.emitBeginWhile(*context, expr.get());
   context->setBreak(pos.get());
   context->setContinue(pos.get());
   ContextPtr terminal = parseIfClause();
   context->builder.emitEndWhile(*context, pos.get(), terminal);
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
      if (!context->returnType->matches(*context->globalData->voidType))
         error(tok,
               SPUG_FSTR("Missing return expression for function "
                          "returning " << context->returnType->name
                         )
               );
      context->builder.emitReturn(*context, 0);
      return;
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
            !context->globalData->voidType->matches(*context->returnType))
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
void Parser::parseImportStmt() {
   ModuleDefPtr mod;
   string canonicalName;
   Token tok = getToken();
   if (tok.isIdent()) {
      toker.putBack(tok);
      vector<string> moduleName;
      parseModuleName(moduleName);
      mod = Crack::loadModule(moduleName, canonicalName);
      if (!mod)
         error(tok, SPUG_FSTR("unable to find module " << canonicalName));
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
         context->builder.loadSharedLibrary(name, syms, *context);
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
         if (context->ns->lookUp(*iter))
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
         context->builder.registerImport(*context, symVal.get());
         context->ns->addAlias(symVal.get());
      }
   }
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
               context->globalData->voidType.get();
         else if (returnType != context->globalData->voidType.get())
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
      } else if (tok.isEQ() || tok.isNE() || tok.isLT() || tok.isLE() || 
                 tok.isGE() || tok.isGT() || tok.isPlus() || tok.isSlash() || 
                 tok.isAsterisk() || tok.isPercent()
                 ) {
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
TypeDefPtr Parser::parseClassDef() {
   Token tok = getToken();
   if (!tok.isIdent())
      unexpected(tok, "Expected class name");
   string className = tok.getData();
   
   // check for an existing definition of the symbol
   checkForExistingDef(tok, tok.getData());

   // parse base class list   
   vector<TypeDefPtr> bases;
   tok = getToken();
   if (tok.isColon())
      while (true) {
         TypeDefPtr baseClass = parseTypeSpec();
         bases.push_back(baseClass);
         
         tok = getToken();
         if (tok.isLCurly())
            break;
         else if (!tok.isComma())
            unexpected(tok, "expected comma or opening brace");
      }
   else if (!tok.isLCurly())
      unexpected(tok, "expected colon or opening brace.");
   
   // if no base classes were specified, and Object has been defined, make 
   // Object the implicit base class.
   if (!bases.size() && context->globalData->objectType)
      bases.push_back(context->globalData->objectType);

   // create a class context
   ContextPtr classContext = new Context(context->builder, 
                                         Context::instance,
                                         context.get(),
                                         0
                                         );

   // emit the beginning of the class, hook it up to the class context and 
   // store a reference to it in the parent context.
   TypeDefPtr type =
      context->builder.emitBeginClass(*classContext, className, bases);
   context->ns->addDef(type.get());
   
   // add the "cast" method
   if (type->hasVTable)
      type->createCast(*classContext);

   // create a lexical context which delegates to both the class context and 
   // the parent context.
   NamespacePtr lexicalNS =
      new CompositeNamespace(type.get(), context->ns.get());
   ContextPtr lexicalContext = new Context(context->builder, 
                                           Context::composite,
                                           classContext.get(),
                                           lexicalNS.get()
                                           );

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
   enum { noPrec, logOrPrec, logAndPrec, cmpPrec, addPrec, multPrec,
          unaryPrec
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
      {"-", addPrec},
      {"==", cmpPrec},
      {"!=", cmpPrec},
      {"<", cmpPrec},
      {">", cmpPrec},
      {"<=", cmpPrec},
      {">=", cmpPrec},
      {"is", cmpPrec},
      {"&&", logAndPrec},
      {"||", logOrPrec},
      
      {0, noPrec}
   };
   
   for (int i = 0; map[i].op; ++i)
      opPrecMap[map[i].op] = map[i].prec;
}   

void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false);
}

// class name { ... }
//             ^     ^
void Parser::parseClassBody() {
   // parse the class body   
   while (true) {
      
      // check for a closing brace or a nested class definition
      Token tok = getToken();
      if (tok.isRCurly()) {
         break;
      } else if (tok.isSemi()) {
         // ignore stray semicolons
         continue;
      } else if (tok.isClass()) {
         TypeDefPtr newType = parseClassDef();
         continue;

      // check for "oper" keyword
      } else if (tok.isOper()) {
         parsePostOper(0);
         continue;
      }
      
      // parse some other kind of definition
      toker.putBack(tok);
      TypeDefPtr type = parseTypeSpec();
      parseDef(type.get());
   }
}

VarDefPtr Parser::checkForExistingDef(const Token &tok, const string &name, 
                                      bool overloadOk
                                      ) {
   ContextPtr classContext;
   VarDefPtr existing = context->ns->lookUp(name);
   if (existing) {
      Namespace *existingNS = existing->owner;
      TypeDef *existingClass = 0;

      // if it's ok to overload, make sure that the existing definition is a 
      // function or an overload def or a stub.
      if (overloadOk && (FuncDefPtr::rcast(existing) || 
                         OverloadDefPtr::rcast(existing) ||
                         StubDefPtr::rcast(existing)
                         )
          )
         return existing;

      // redefinition in the same context is an error
      if (existingNS == context->ns.get())
         redefineError(tok, existing.get());
      // redefinition in a derived context is fine, but if we're not in a 
      // derived context display a warning.  TODO: if this check doesn't need 
      // to be this way, replace it with something that makes sense.
      else if (!(classContext = context->getClassContext()) ||
               !classContext->returnType || 
               !(existingClass = TypeDefPtr::cast(existingNS)) ||
               !existingClass->matches(*classContext->returnType)
               ) {
         warn(tok,
              SPUG_FSTR("Symbol " << name << 
                         " hides another definition in an enclosing context."
                        )
              );
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
   Location loc = tok.getLocation();
   stringstream text;
   text << loc.getName() << ':' << loc.getLineNumber() << ": " << msg;
   throw ParseError(text.str().c_str());
}

void Parser::warn(const Token &tok, const std::string & msg) {
   Location loc = tok.getLocation();
   cerr << loc.getName() << ":" << loc.getLineNumber() << ": " << msg << endl;
}
