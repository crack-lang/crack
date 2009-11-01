
#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include <spug/StringFmt.h>
#include "model/ArgDef.h"
#include "model/Branchpoint.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/FuncCall.h"
#include "model/Expr.h"
#include "model/IntConst.h"
#include "model/StrConst.h"
#include "model/TypeDef.h"
#include "model/VarDef.h"
#include "model/VarRef.h"
#include "builder/Builder.h"
#include "ParseError.h"

using namespace std;
using namespace parser;
using namespace model;

void Parser::unexpected(const Token &tok, const char *userMsg) {
   Location loc = tok.getLocation();
   stringstream msg;
   msg << "token " << tok.getData() << " was not expected at this time";

   // add the user message, if provided
   if (userMsg)
      msg << ", " << userMsg;

   error(tok, msg.str());
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
      parseReturnStmt();
      return true;
   }

   ExprPtr expr;
   if (tok.isIdent()) {
      
      // if the identifier is a type, try to parse a definition
      VarDefPtr def = context->lookUp(tok.getData());
      if (def && dynamic_cast<TypeDef *>(def.obj) &&
          parseDef(TypeDefPtr::dcast(def))
          ) {
         if (!defsAllowed)
            error(tok, "definition is not allowed in this context");
         return false;
      } else if (!def) {
         // unknown identifier
         // XXX if the next token(s) are '=' or ':=' then this is an 
         // assignment
         assert(false);
      } else {
         toker.putBack(tok);
         expr = parseExpression("; }");
      }

   } else {
      toker.putBack(tok);
      expr = parseExpression("; }");
   }

   // if we got an expression, emit it.
   if (expr)
      expr->emit(*context);

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
      if (tok.isRCurly()) {
         if (nested)
            return gotTerminalStatement;
         else
            unexpected(tok, "expected statement or closing brace.");
      } else if (tok.isEnd()) {
         if (!nested)
            return gotTerminalStatement;
	 else
	    unexpected(tok, "expected statement or end-of-file");
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

bool Parser::isBinaryOperator(const Token &tok) {
   if (tok.isPlus() || tok.isMinus() || tok.isAsterisk() || tok.isSlash() || 
       tok.isPercent())
      return true;
   else
      return false;
}

ExprPtr Parser::parseExpression(const char *terminators) {

   ExprPtr expr;

   // check for a method
   Token tok = toker.getToken();
   if (tok.isIdent()) {

      // is it an assignment?
      Token tok1 = toker.getToken();
      if (tok1.isAssign()) {

         // look up the variable
         VarDefPtr var = context->lookUp(tok.getData());
         if (!var)
            error(tok1,
                  SPUG_FSTR("attempted to assign undefined variable " <<
                            tok.getData()
                            )
                  );
         
         // XXX need to verify that it's not a constant (or at least not a 
         // function)

	 // start the method with an augmented name

	 // parse an expression
	 ExprPtr expr = parseExpression(terminators);
	 if (!expr) {
	    tok1 = toker.getToken();
	    error(tok1, "expression expected");
	 }

         // make sure the types are compatible	 
         if (!var->type->matches(*expr->type))
            error(tok,
                  SPUG_FSTR("Cannot assign a value of type " <<
                            expr->type->name <<
                            " to a variable of type " <<
                            var->type->name
                            )
                  );
         
         var->emitAssignment(*context, expr);

      } else if (tok1.isLParen()) {
         // method invocation
         // XXX needs to handle overloaded functions.
         FuncDefPtr func = FuncDefPtr::dcast(context->lookUp(tok.getData()));
	 FuncCallPtr funcCall = context->builder.createFuncCall(func);
	 parseMethodArgs(funcCall->args);
         expr = ExprPtr::ucast(funcCall);
      } else {
         // for anything else, it's a variable
         toker.putBack(tok1);
         VarDefPtr def = context->lookUp(tok.getData());
         if (!def)
            error(tok,
                  SPUG_FSTR("Undefined variable: " << tok.getData()));

         // XXX def could be a generic class and generic classes require 
         // special magic to allow us to parse the <>'s
         VarDefPtr varDef = VarDefPtr::dcast(def);
         expr = ExprPtr::ucast(context->builder.createVarRef(varDef));
      }

      // close off the method no matter how we started it
      tok1 = toker.getToken();
      toker.putBack(tok1);
   } else if (tok.isString()) {
      expr = context->getStrConst(tok.getData());
   } else if (tok.isInteger()) {
      expr = context->builder.createIntConst(*context, 
                                             atoi(tok.getData().c_str())
                                             );
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

	 // process the method XXX need to add the "self" expression
	 FuncDefPtr func = FuncDefPtr::dcast(context->lookUp(tok.getData()));
	 FuncCallPtr funcCall = context->builder.createFuncCall(func);
	 parseMethodArgs(funcCall->args);

      } else if (isBinaryOperator(tok)) {
         // parse the right-hand-side expression
         ExprPtr rhs = parseExpression(terminators);
         
         FuncCall::ExprVector exprs(2);
         exprs[0] = expr;
         exprs[1] = rhs;
         char name[7] = { 'o', 'p', 'e', 'r', ' ', tok.getData()[0] };
         FuncDefPtr func = context->lookUp(name, exprs);
         if (!func)
            error(tok,
                  SPUG_FSTR("Operator " << expr->type->name << " " <<
                            tok.getData() << " " << rhs->type->name <<
                            " undefined."
                            )
                  );
         FuncCallPtr funcCall = context->builder.createFuncCall(func);
         funcCall->args = exprs;
         expr = ExprPtr::ucast(funcCall);
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
void Parser::parseMethodArgs(FuncCall::ExprVector &args) {
     
   Token tok = toker.getToken();
   while (true) {
      if (tok.isRParen())
         return;
         
      // XXX should be verifying arg types against signature

      // get the next argument value
      toker.putBack(tok);
      ExprPtr arg = parseExpression(",)");
      args.push_back(arg);

      // comma signals another argument
      tok = toker.getToken();
      if (!tok.isComma())
	 break;
   }
}

TypeDefPtr Parser::parseTypeSpec() {
   Token tok = toker.getToken();
   if (!tok.isIdent())
      unexpected(tok, "type identifier expected");
   
   VarDefPtr def = context->lookUp(tok.getData());
   TypeDef *typeDef = dynamic_cast<TypeDef *>(def.obj);
   if (!typeDef)
      error(tok, SPUG_FSTR(tok.getData() << " is not a type."));
   
   // XXX need to deal with compound types
   
   return typeDef;
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
      if (context->lookUp(varName))
         warn(tok, SPUG_FSTR("Variable " << varName << " redefined."));

      // XXX need to check for a default variable assignment
      
      ArgDefPtr argDef = context->builder.createArgDef(argType, varName);
      args.push_back(argDef);
      context->addDef(VarDefPtr::ucast(argDef));
      
      // check for a comma
      tok = toker.getToken();
      if (tok.isComma())
         tok = toker.getToken();
      else if (!tok.isRParen())
         unexpected(tok, "expected ',' or ')' after argument definition");
   }
}

// type var = initializer, var2 ;
//     ^                         ^
// type function() { }
//     ^              ^
bool Parser::parseDef(const TypeDefPtr &type) {
   Token tok2 = toker.getToken();
   // XXX if it's '<', make sure the type is a generic and parse 
   // nested types.
   
   if (tok2.isIdent()) {
      string varName = tok2.getData();
      // make sure we're not hiding anything else
      if (context->lookUp(varName))
         warn(tok2,
              SPUG_FSTR("Variable " << varName << " redefined." )
              );

      // this could be a variable or a function
      Token tok3 = toker.getToken();
      if (tok3.isSemi()) {
         // it's a variable.  Emit a variable definition and store it 
         // in the context.
         VarDefPtr varDef = 
            type->emitVarDef(*context, varName, 0);
         context->addDef(varDef);
         return true;
      } else if (tok3.isAssign()) {
         ExprPtr initializer;

         // check for a curly brace, indicating construction args.
         Token tok4 = toker.getToken();
         if (tok4.isLCurly()) {
            // XXX need to deal with construction args
            assert(false);
         } else {
            toker.putBack(tok4);
            initializer = parseExpression(";,");
            
            // XXX if this is a comma, we need to go back and parse 
            // another definition for the type.
            tok4 = toker.getToken();
            assert(tok4.isSemi());
         }

         // make sure the initializer matches the declared type.
         if (!type->matches(*initializer->type))
            error(tok4, "Incorrect type for initializer.");
            
         VarDefPtr varDef = type->emitVarDef(*context, varName,
                                             initializer
                                             );
         context->addDef(varDef);
         return true;
      } else if (tok3.isLParen()) {
         // function definition

         // push a new context, arg defs will be stored in the new context.
         pushContext(context->createSubContext(Context::local));
         context->returnType = type;

         // parse the arguments
         vector<ArgDefPtr> argDefs;
         parseArgDefs(argDefs);
         
         tok3 = toker.getToken();
         if (!tok3.isLCurly())
            unexpected(tok3, "expected '{' in function definition");
         
         // parse the body
         FuncDefPtr funcDef =
            context->builder.emitBeginFunc(*context, varName, type, argDefs);
         bool terminal = parseBlock(true);
         
         // if the block doesn't always terminate, either give an error or 
         // return void if the function return type is void
         if (!terminal)
            if (context->globalData->voidType->matches(*context->returnType))
               context->builder.emitReturn(*context, 0);
            else
               // XXX we don't have the closing curly brace location, 
               // currently reporting the error on the top brace
               error(tok3, "missing return statement for non-void function.");

         context->builder.emitEndFunc(*context, funcDef);
         popContext();
         
         // store the new definition in the context.
         context->addDef(VarDefPtr::ucast(funcDef));
         
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
      pushContext(context->createSubContext());
      bool terminal = parseBlock(true);
      popContext();
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
   
   ExprPtr cond = parseExpression(")");
   if (!context->globalData->boolType->matches(*cond->type))
      error(tok, "Condition is not boolean.");
   
   tok = toker.getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected closing paren");
   
   BranchpointPtr pos = context->builder.emitIf(*context, cond);

   bool terminalIf = parseIfClause();
   bool terminalElse = false;

   // check for the "else"
   tok = toker.getToken();
   if (tok.isElse()) {
      pos = context->builder.emitElse(*context, pos);
      terminalElse = parseIfClause();
      context->builder.emitEndIf(*context, pos);
   } else {
      toker.putBack(tok);
      context->builder.emitEndIf(*context, pos);
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
   
   ExprPtr expr = parseExpression(")");
   tok = toker.getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected right paren after conditional expression");
   
   BranchpointPtr pos = context->builder.emitBeginWhile(*context, expr);
   bool terminal = parseIfClause();
   context->builder.emitEndWhile(*context, pos);
   return terminal;
}

void Parser::parseReturnStmt() {
   // parse the return expression, make sure that it matches the return type.
   Token tok = toker.getToken();
   if (tok.isSemi()) {
      return;
   } else if (tok.isEnd() || tok.isRCurly()) {
      toker.putBack(tok);
      return;
   }

   toker.putBack(tok);
   ExprPtr expr = parseExpression("; }");
   if (!context->returnType->matches(*expr->type))
      error(tok,
            SPUG_FSTR("Invalid return type " << expr->type->name <<
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
   
   context->builder.emitReturn(*context, expr);

   tok = toker.getToken();   
   if (tok.isEnd() || tok.isRCurly())
      toker.putBack(tok);
   else if (!tok.isSemi())
      unexpected(tok, "expected semicolon or block terminator");
}

void Parser::pushContext(const model::ContextPtr &newContext) {
   context = newContext;
}

void Parser::popContext() {
   assert(context->parent);
   context = context->parent;
}

void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false);
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
