// Copyright 2009 Google Inc.

#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include <spug/Exception.h>
#include <spug/StringFmt.h>
#include "model/ArgDef.h"
#include "model/AssignExpr.h"
#include "model/Branchpoint.h"
#include "model/CleanupFrame.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/FuncCall.h"
#include "model/Expr.h"
#include "model/IntConst.h"
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
   context->getDefContext()->addDef(varDef);
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
   Token tok = toker.getToken();
   if (tok.getType() != type)
      unexpected(tok, error);
}

bool Parser::parseStatement(bool defsAllowed) {
   // peek at the next token
   Token tok = toker.getToken();

   // check for statements
   if (tok.isIf()) {
      return parseIfStmt();
   } else if (tok.isWhile()) {
      return parseWhileStmt();
   } else if (tok.isElse()) {
      unexpected(tok, "'else' with no matching 'if'");
   } else if (tok.isReturn()) {
      if (context->scope == Context::module)
         error(tok, "Return statement not allowed in module scope");
      parseReturnStmt();
      return true;
   } else if (tok.isImport()) {
      parseImportStmt();
      return false;
   } else if (tok.isClass()) {
      if (!defsAllowed)
         error(tok, "class definitions are not allowed in this context");
      parseClassDef();
      
      // check for a semicolon or another terminator, if it's something else 
      // parse some variable definitions.
      tok = toker.getToken();
      if (tok.isSemi()) {
         return false;
      } else {
         toker.putBack(tok);
         if (tok.isRCurly() || tok.isEnd())
            return false;
      }
   }

   ExprPtr expr;
   if (tok.isIdent()) {
      
      // if the identifier is a type, try to parse a definition
      VarDefPtr def = context->lookUp(tok.getData());
      TypeDef *typeDef = TypeDefPtr::rcast(def);
      if (typeDef && parseDef(typeDef)) {
         if (!defsAllowed)
            error(tok, "definition is not allowed in this context");
         return false;
      } else if (!def) {
         // XXX think I want to move this into expression, it's just as valid 
         // for an existing identifier (in a parent context) as for a 
         // non-existing one.
         // unknown identifier. if the next token(s) is ':=' (the "define" 
         // operator) then this is an assignment
         Token tok2 = toker.getToken();
         if (tok2.isDefine()) {
            if (!defsAllowed)
               error(tok, "definition is not allowed in this context.");
            expr = parseExpression();
            context->createCleanupFrame();
            VarDefPtr varDef =
               expr->type->emitVarDef(*context, tok.getData(), expr.get());
            context->closeCleanupFrame();
            addDef(varDef.get());
            context->cleanupFrame->addCleanup(varDef.get());
            
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
   tok = toker.getToken();
   if (tok.isEnd() || tok.isRCurly())
      toker.putBack(tok);
   else if (!tok.isSemi())
      unexpected(tok, "expected semicolon or a block terminator");

   return false;
}

bool Parser::parseBlock(bool nested) {
   Token tok;

   // this gets set to true if we encounter a "terminal statement" which is a 
   // statement that will always return or throw an exception.
   bool gotTerminalStatement = false;
   // keeps track of whether we've emitted a warning about stuff after a 
   // terminal statement.
   bool gotStuffAfterTerminalStatement = false;

   while (true) {

      // peek at the next token
      tok = toker.getToken();

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
         if (!gotTerminalStatement && nested)
            context->builder.closeAllCleanups(*context);
         return gotTerminalStatement;
      }
      
      toker.putBack(tok);
      
      // if we already got a terminal statement, anything else is just dead 
      // code - warn them about the first thing.
      if (gotTerminalStatement && !gotStuffAfterTerminalStatement) {
         warn(tok, "unreachable code");
         gotStuffAfterTerminalStatement = true;
      }

      gotTerminalStatement |= parseStatement(true);
   }
}

ExprPtr Parser::makeThisRef(const Token &ident) {
   VarDefPtr thisVar = context->lookUp("this");
   if (!thisVar)
      error(ident,
            SPUG_FSTR("instance member " << ident.getData() <<
                       " may not be used in a static context."
                      )
            );
                      
   return context->builder.createVarRef(thisVar.get());
}

ExprPtr Parser::createVarRef(Expr *container, const Token &ident) {
   Context &varContext = container ? *container->type->context : *context;
   VarDefPtr var = varContext.lookUp(ident.getData());
   if (!var)
      error(ident,
            SPUG_FSTR("Undefined variable: " << ident.getData()));

   // if the definition is for an instance variable, emit an implicit 
   // "this" dereference.  Otherwise just emit the variable
   if (var->context->scope == Context::instance) {
      // if there's no container, try to use an implicit "this"
      ExprPtr receiver = container ? container : makeThisRef(ident);
      return context->builder.createFieldRef(receiver.get(), var.get());
   } else {
      return context->builder.createVarRef(var.get());
   }
}

ExprPtr Parser::parsePostIdent(Expr *container, const Token &ident) {
   Context &varContext = container ? *container->type->context : *context;
   // is it an assignment?
   Token tok1 = toker.getToken();
   if (tok1.isAssign()) {

      VarDefPtr var = varContext.lookUp(ident.getData());
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
         tok1 = toker.getToken();
         error(tok1, "expression expected");
      }

      // if this is an instance variable, emit a field assignment.  
      // Otherwise emit a normal variable assignment.
      if (var->context->scope == Context::instance) {
         // if there's no container, try to use an implicit "this"
         ExprPtr receiver = container ? container : makeThisRef(ident);
         return AssignExpr::create(*context, ident, receiver.get(), var.get(), 
                                   val.get()
                                   );
      } else {
         return AssignExpr::create(*context, ident, var.get(), val.get());
      }

   } else if (tok1.isLParen()) {
      // function/method invocation
      
      // parse the arg list
      FuncCall::ExprVec args;
      parseMethodArgs(args);
      
      // look up the variable
      
      // lookup the method from the variable context's type context
      // XXX needs to handle callable objects.
      FuncDefPtr func = varContext.lookUp(*context, ident.getData(), args);
      if (!func)
         error(ident,
               SPUG_FSTR("No method exists matching " << ident.getData() <<
                          " with these argument types."
                         )
               );

      // if the definition is for an instance variable, emit an implicit 
      // "this" dereference.  Otherwise just emit the variable
      ExprPtr receiver;
      if (func->flags & FuncDef::method)
         // if there's no container, try to use an implicit "this"
         receiver = container ? container : makeThisRef(ident);

      FuncCallPtr funcCall = context->builder.createFuncCall(func.get());
      funcCall->args = args;
      funcCall->receiver = receiver;
      return funcCall;
   } else {
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
   while (!(tok = toker.getToken()).isIstrEnd()) {
      ExprPtr arg;
      if (tok.isString()) {
         arg = context->getStrConst(tok.getData());
      } else if (tok.isIdent()) {
         // get a variable definition
         arg = createVarRef(0, tok);
         toker.continueIString();
      } else if (tok.isLParen()) {
         arg = parseExpression();
         tok = toker.getToken();
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
      FuncDefPtr func = expr->type->context->lookUp(*context, "format", args);
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
   

ExprPtr Parser::parseExpression(unsigned precedence) {

   ExprPtr expr;

   // check for null
   Token tok = toker.getToken();
   if (tok.isNull()) {
      expr = new NullConst(context->globalData->voidPtrType.get());
   
   // check for a nested parenthesized expression
   } else if (tok.isLParen()) {
      expr = parseExpression();
      tok = toker.getToken();
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
   // for the unary "!" operator
   } else if (tok.isBang()) {
      FuncCall::ExprVec args(1);
      args[0] = parseExpression(getPrecedence(tok.getData()));
      FuncDefPtr funcDef = context->lookUp(*context, "oper !", args);
      if (!funcDef)
         error(tok, "No ! operator exists for this type.");
      FuncCallPtr funcCall = context->builder.createFuncCall(funcDef.get());
      funcCall->args = args;
      expr = funcCall;
   } else if (tok.isLCurly()) {
      assert(false);
   } else {
      unexpected(tok, "expected an expression");
   }

   // parse any following secondary expressions...
   tok = toker.getToken();
   while (true) {
      if (tok.isDot()) {
	 // get the next token, which should be an identifier
	 tok = toker.getToken();
	 if (!tok.isIdent())
	    error(tok, "identifier expected");

         expr = parsePostIdent(expr.get(), tok);
      } else if (tok.isLBracket()) {
         // the array indexing operators
         FuncCall::ExprVec args(1);
         args[0] = parseExpression();
         
         // parse the right bracket
         Token tok2 = toker.getToken();
         if (!tok2.isRBracket())
            unexpected(tok2, "expected right bracket.");
         
         // check for an assignment operator
         tok2 = toker.getToken();
         FuncCallPtr funcCall;
         if (tok2.isAssign()) {
            // this is "a[i] = v"
            args.push_back(parseExpression());
            FuncDefPtr funcDef =
               expr->type->context->lookUp(*context, "oper []=", args);
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
               expr->type->context->lookUp(*context, "oper []", args);
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
         FuncDefPtr func = context->lookUp(*context, name, exprs);
         if (!func)
            error(tok,
                  SPUG_FSTR("Operator " << expr->type->name << " " <<
                            tok.getData() << " " << rhs->type->name <<
                            " undefined."
                            )
                  );
         FuncCallPtr funcCall = context->builder.createFuncCall(func.get());
         funcCall->args = exprs;
         expr = funcCall;
      } else if (tok.isIstrBegin()) {
         expr = parseIString(expr.get());
      } else {
	 // next token is not part of the expression
	 break;
      }

      // get the next token
      tok = toker.getToken();

   }
   toker.putBack(tok);
   return expr;
}

// func( arg, arg)
//      ^         ^
void Parser::parseMethodArgs(FuncCall::ExprVec &args) {
     
   Token tok = toker.getToken();
   while (true) {
      if (tok.isRParen())
         return;
         
      // XXX should be verifying arg types against signature

      // get the next argument value
      toker.putBack(tok);
      ExprPtr arg = parseExpression();
      args.push_back(arg);

      // comma signals another argument
      tok = toker.getToken();
      if (tok.isComma())
         tok = toker.getToken();
   }
}

TypeDefPtr Parser::parseTypeSpec() {
   Token tok = toker.getToken();
   if (!tok.isIdent())
      unexpected(tok, "type identifier expected");
   
   VarDefPtr def = context->lookUp(tok.getData());
   TypeDef *typeDef = TypeDefPtr::rcast(def);
   if (!typeDef)
      error(tok, SPUG_FSTR(tok.getData() << " is not a type."));
   
   // XXX need to deal with compound types
   
   return typeDef;
}

void Parser::parseModuleName(vector<string> &moduleName) {
   Token tok = toker.getToken();
   while (true) {
      moduleName.push_back(tok.getData());
      tok = toker.getToken();
      if (!tok.isDot()) {
         toker.putBack(tok);
         return;
      }
      
      tok = toker.getToken();
      if (!tok.isIdent())
         unexpected(tok, "identifier expected");
   }
}

// type funcName ( type argName, ... ) {
//                ^                   ^
void Parser::parseArgDefs(vector<ArgDefPtr> &args) {

   // load the next token so we can check for the immediate closing paren of 
   // an empty argument list.
   Token tok = toker.getToken();
      
   while (!tok.isRParen()) {

      // parse the next argument type
      toker.putBack(tok);
      TypeDefPtr argType = parseTypeSpec();
      
      tok = toker.getToken();
      if (!tok.isIdent())
         error(tok, "identifier (argument name) expected.");
      
      // make sure we're not redefining an existing variable
      // XXX complain extra loud if the variable is defined in the current 
      // context (and make sure that the current context is the function 
      // context)
      std::string varName = tok.getData();
      checkForExistingDef(tok);

      // XXX need to check for a default variable assignment
      
      ArgDefPtr argDef = context->builder.createArgDef(argType.get(), varName);
      args.push_back(argDef);
      addDef(argDef.get());
      
      // check for a comma
      tok = toker.getToken();
      if (tok.isComma())
         tok = toker.getToken();
      else if (!tok.isRParen())
         unexpected(tok, "expected ',' or ')' after argument definition");
   }
}

void Parser::parseFuncDef(TypeDef *returnType, const Token &nameTok,
                          const string &name,
                          bool initializers
                          ) {
   // check for an existing, non-function definition.
   VarDefPtr existingDef = checkForExistingDef(nameTok, true);

   // if this is a class context, we're defining a method.
   ContextPtr classCtx = context->getClassContext();
   bool isMethod = classCtx ? true : false;

   // push a new context, arg defs will be stored in the new context.
   ContextPtr subCtx = context->createSubContext(Context::local);
   ContextStackFrame cstack(*this, subCtx.get());
   context->returnType = returnType;
   context->toplevel = true;
   
   // if this is a method, add the "this" variable
   if (isMethod) {
      assert(classCtx && "method not in class context.");
      ArgDefPtr argDef =
         context->builder.createArgDef(classCtx->returnType.get(), 
                                       "this"
                                       );
      addDef(argDef.get());
   }

   // parse the arguments
   vector<ArgDefPtr> argDefs;
   parseArgDefs(argDefs);
   
   Token tok3 = toker.getToken();
   if (tok3.isSemi()) {
      // abstract or forward declaration - see if we've got a stub 
      // definition
      StubDef *stub;
      if (existingDef && (stub = StubDefPtr::rcast(existingDef))) {
         FuncDefPtr funcDef =
            context->builder.createExternFunc(*context, FuncDef::noFlags,
                                              name,
                                              returnType,
                                              argDefs,
                                              stub->address
                                              );
         stub->context->removeDef(stub);
         cstack.restore();
         addDef(funcDef.get());
         return;
      } else {
         // XXX forward declaration
         error(tok3, 
               "abstract/forward declarations are not supported yet");
      }
   } else if (!tok3.isLCurly()) {
      unexpected(tok3, "expected '{' in function definition");
   }

   bool isVirtual = isMethod && classCtx->returnType->hasVTable && 
                    !TypeDef::isImplicitFinal(name);

   // If we're overriding/implementing a previously declared virtual 
   // function, we'll store it here.
   FuncDefPtr override;

   // XXX need to consolidate FuncDef and OverloadDef
   // we now need to verify that the new definition doesn't hide an 
   // existing definition.
   FuncDef *existingFuncDef = FuncDefPtr::rcast(existingDef);
   if (existingFuncDef && existingFuncDef->matches(argDefs)) {
      if (!(context->getDefContext() == existingDef->context) ||
          !existingFuncDef->isOverridable()
          )
         override = existingFuncDef;
      else
         error(nameTok,
               SPUG_FSTR("Definition of " << name <<
                        " hides previous overload."
                        )
               );
   } else {
      OverloadDef *existingOvldDef = OverloadDefPtr::rcast(existingDef);
      if (existingOvldDef && 
         (override = existingOvldDef->getSigMatch(argDefs)) &&
         !override->isOverridable()
         ) {
         error(nameTok,
               SPUG_FSTR("Definition of " << name <<
                        " hides previous overload."
                        )
               );
      }

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
      ContextStackFrame cstack(*this, context->parents[0].get());
      addDef(funcDef.get());
   }
   
   bool terminal = parseBlock(true);
   
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
}         
         

// type var = initializer, var2 ;
//     ^                         ^
// type function() { }
//     ^              ^
bool Parser::parseDef(TypeDef *type) {
   Token tok2 = toker.getToken();
   // XXX if it's '<', make sure the type is a generic and parse 
   // nested types.
   
   if (tok2.isIdent()) {
      string varName = tok2.getData();

      // this could be a variable or a function
      Token tok3 = toker.getToken();
      if (tok3.isSemi()) {
         // it's a variable.

         // make sure we're not hiding anything else
         checkForExistingDef(tok2);
         
         // make sure we've got a default initializer
         if (!type->defaultInitializer)
            error(tok2, "no default constructor");
         
         // Emit a variable definition and store it in the context (in a 
         // cleanup frame so transient initializers get destroyed here)
         context->createCleanupFrame();
         VarDefPtr varDef = type->emitVarDef(*context, varName, 0);
         context->closeCleanupFrame();
         addDef(varDef.get());
         context->cleanupFrame->addCleanup(varDef.get());
         return true;
      } else if (tok3.isAssign()) {
         ExprPtr initializer;

         // make sure we're not hiding anything else
         checkForExistingDef(tok2);

         // check for a curly brace, indicating construction args.
         Token tok4 = toker.getToken();
         if (tok4.isLCurly()) {
            // XXX need to deal with construction args
            assert(false);
         } else {
            toker.putBack(tok4);
            initializer = parseExpression();
            
            // XXX if this is a comma, we need to go back and parse 
            // another definition for the type.
            tok4 = toker.getToken();
            assert(tok4.isSemi());
         }

         // make sure the initializer matches the declared type.
         initializer = initializer->convert(*context, type);
         if (!initializer)
            error(tok4, "Incorrect type for initializer.");
         
         context->createCleanupFrame();
         VarDefPtr varDef = type->emitVarDef(*context, varName,
                                             initializer.get()
                                             );
         context->closeCleanupFrame();
         addDef(varDef.get());
         context->cleanupFrame->addCleanup(varDef.get());
         return true;
      } else if (tok3.isLParen()) {
         // function definition
         parseFuncDef(type, tok2, tok2.getData(), false);
         return true;
      } else {
         unexpected(tok3,
                    "expected variable initializer or function "
                    "definition."
                    );
      }
   } else {
      unexpected(tok2, "expected variable definition");
   }
   
   return false;
}

bool Parser::parseIfClause() {
   Token tok = toker.getToken();
   if (tok.isLCurly()) {
      ContextStackFrame cstack(*this, context->createSubContext().get());
      bool terminal = parseBlock(true);
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
bool Parser::parseIfStmt() {
   Token tok = toker.getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after if");
   
   TypeDef *boolType = context->globalData->boolType.get();
   ExprPtr cond = parseExpression()->convert(*context, boolType);
   if (!cond)
      error(tok, "Condition is not boolean.");
   
   tok = toker.getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected closing paren");
   
   BranchpointPtr pos = context->builder.emitIf(*context, cond.get());

   bool terminalIf = parseIfClause();
   bool terminalElse = false;

   // check for the "else"
   tok = toker.getToken();
   if (tok.isElse()) {
      pos = context->builder.emitElse(*context, pos.get(), terminalIf);
      terminalElse = parseIfClause();
      context->builder.emitEndIf(*context, pos.get(), terminalElse);
   } else {
      toker.putBack(tok);
      context->builder.emitEndIf(*context, pos.get(), terminalIf);
   }

   // the if is terminal if both conditions are terminal   
   return terminalIf && terminalElse;
}

// while ( expr ) stmt ; (';' can be replaced with EOF)
//      ^               ^
// while ( expr ) { ... }
//      ^                ^
bool Parser::parseWhileStmt() {
   Token tok = toker.getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after while");
   
   ExprPtr expr = parseExpression();
   tok = toker.getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected right paren after conditional expression");
   
   BranchpointPtr pos = context->builder.emitBeginWhile(*context, expr.get());
   bool terminal = parseIfClause();
   context->builder.emitEndWhile(*context, pos.get());
   return terminal;
}

void Parser::parseReturnStmt() {
   // check for a return with no expression
   Token tok = toker.getToken();
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

   tok = toker.getToken();   
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
   Token tok = toker.getToken();
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
      tok = toker.getToken();
      if (tok.isIdent()) {
         syms.push_back(tok.getData());
         tok = toker.getToken();
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
         if (context->lookUp(*iter))
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
         context->addAlias(symVal.get());
      }
   }
}

// oper name ( args ) { ... }
//     ^                     ^
void Parser::parsePostOper(TypeDef *returnType) {

   Token tok = toker.getToken();
   if (tok.isIdent()) {
      const string &ident = tok.getData();
      if (ident == "init") {
         // oper init must be of type "void"
         if (!returnType)
            context->returnType = returnType =
               context->globalData->voidType.get();
         else if (returnType != context->globalData->voidType.get())
            error(tok, "oper init must be of return type 'void'");
         expectToken(Token::lparen, "expected argument list");

         parseFuncDef(returnType, tok, "oper init", true);
      } else {
         unexpected(tok, "only 'oper init' honored at this time");
      }

   } else {
      unexpected(tok, "identifier expected after 'oper' keyword");
   }
}

// class name : base, base { ... }
//      ^                         ^
TypeDefPtr Parser::parseClassDef() {
   Token tok = toker.getToken();
   if (!tok.isIdent())
      unexpected(tok, "Expected class name");
   string className = tok.getData();
   
   // check for an existing definition of the symbol
   checkForExistingDef(tok);

   // parse base class list   
   vector<TypeDefPtr> bases;
   tok = toker.getToken();
   if (tok.isColon())
      while (true) {
         TypeDefPtr baseClass = parseTypeSpec();
         bases.push_back(baseClass);
         
         tok = toker.getToken();
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

   // create a class context, and a lexical context which delegates to both 
   // the class context and the parent context.
   ContextPtr classContext = new Context(context->builder, 
                                         Context::instance,
                                         context->globalData
                                         );
   ContextPtr lexicalContext = new Context(context->builder, 
                                           Context::composite,
                                           context->globalData
                                           );
   lexicalContext->parents.push_back(classContext);
   lexicalContext->parents.push_back(context);

   // store the base classes as the parent contexts of the class context.  
   // Track whether there's a vtable while we're at it.
   bool gotVTable = false;
   for (vector<TypeDefPtr>::iterator iter = bases.begin();
        iter != bases.end();\
        ++iter
        ) {
      classContext->parents.push_back((*iter)->context);
      if ((*iter)->hasVTable)
         gotVTable = true;
   }

   // push the context
   ContextStackFrame cstack(*this, lexicalContext.get());
   
   // emit the beginning of the class, hook it up to the class context and 
   // store a reference to it in the parent context.
   TypeDefPtr type = classContext->returnType =
      context->builder.emitBeginClass(*classContext, className, bases);
   type->context = classContext;
   type->hasVTable = gotVTable;
   cstack.parent().getDefContext()->addDef(type.get());

   // parse the class body   
   while (true) {
      
      // check for a closing brace or a nested class definition
      tok = toker.getToken();
      if (tok.isRCurly()) {
         break;
      } else if (tok.isClass()) {
         TypeDefPtr newType = parseClassDef();
         tok = toker.getToken();
         if (tok.isRCurly()) {
            break;
         } else if (tok.isSemi()) {
            continue;
         } else {
            // deal with this class as the return type var type of the next 
            // definition.
            toker.putBack(tok);
            parseDef(newType.get());
            continue;
         }
      
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

   type->rectify();
   classContext->complete = true;
   classContext->builder.emitEndClass(*classContext);
   cstack.restore();
   
   return type;
}

Parser::Parser(Toker &toker, model::Context *context) : 
   toker(toker),
   moduleCtx(context),
   context(context) {
   
   // build the precedence table
   struct { const char *op; unsigned prec; } map[] = {
      {"!", 4},
      {"*", 3},
      {"/", 3},
      {"%", 3},
      {"+", 2},
      {"-", 2},
      {"==", 1},
      {"!=", 1},
      {"<", 1},
      {">", 1},
      {"<=", 1},
      {">=", 1},
      {"is", 1},
      {0, 0}
   };
   
   for (int i = 0; map[i].op; ++i)
      opPrecMap[map[i].op] = map[i].prec;
}   

void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false);
}

VarDefPtr Parser::checkForExistingDef(const Token &tok, bool overloadOk) {
   ContextPtr classContext;
   VarDefPtr existing = context->lookUp(tok.getData());
   if (existing) {
      Context *existingContext = existing->context;

      // if it's ok to overload, make sure that the existing definition is a 
      // function or an overload def or a stub.
      if (overloadOk && (FuncDefPtr::rcast(existing) || 
                         OverloadDefPtr::rcast(existing) ||
                         StubDefPtr::rcast(existing)
                         )
          )
         return existing;

      // redefinition in the same context is an error
      if (existingContext == context)
         error(tok, 
               SPUG_FSTR("Symbol " << tok.getData() <<
                          " is already defined in this context."
                         )
               );
      // redefinition in a derived context is fine, but if we're not in a 
      // derived context display a warning.
      else if (!(classContext = context->getClassContext()) ||
               !classContext->returnType || !existingContext->returnType ||
               !existingContext->returnType->matches(*classContext->returnType)
               ) {
         warn(tok,
              SPUG_FSTR("Symbol " << tok.getData() << 
                         " hides another definition in an enclosing context."
                        )
              );
      }
   }
   
   return 0;
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
