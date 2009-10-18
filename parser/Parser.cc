
#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include <spug/StringFmt.h>
#include "model/Branchpoint.h"
#include "model/Context.h"
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

   error(tok, msg.str().c_str());
}

void Parser::parseBlock(bool nested) {
   Token tok;

   while (true) {

      // peek at the next token
      tok = toker.getToken();
      
      // check for statements
      if (tok.isIf()) {
         parseIfStmt();
         continue;
      } else if (tok.isWhile()) {
         parseWhileStmt();
         continue;
      } else if (tok.isElse()) {
         unexpected(tok, "'else' with no matching 'if'");
      }

      ExprPtr expr;
      if (tok.isIdent()) {
         
         // if the identifier is a type, try to parse a definition
         VarDefPtr def = context->lookUp(tok.getData());
         if (def && dynamic_cast<TypeDef *>(def.obj) &&
             parseDef(TypeDefPtr::dcast(def))
             ) {
            continue;
         } else if (!def) {
            // unknown identifier
            // XXX if the next token(s) are '=' or ':=' then this is an 
            // assignment
            assert(false);
         } else {
            toker.putBack(tok);
            expr = parseExpression(nested ? "; " : ";}");
         }

      } else {
	 toker.putBack(tok);
	 expr = parseExpression(nested ? "; " : ";}");
      }

      // if we got an expression, emit it.
      if (expr)
         expr->emit(*context);

      // check for a semicolon
      tok = toker.getToken();
      if (tok.isSemi()) {
	 continue;

      // check for a different block terminator depending on whether we are
      // nested or not.
      } else if (nested) {
	 if (tok.isRCurly()) {
	    return;
	 } else {
	    unexpected(tok, "expected semicolon or closing curly-bracket");
	 }
      } else {
	 if (tok.isEnd()) {
	    return;
	 } else {
	    unexpected(tok, "expected semicolon or end-of-file");
	 }
      }
   }
}

bool Parser::isBinaryOperator(const Token &tok) {
   if (tok.isPlus())
      return true;
   else
      return false;
}

ExprPtr Parser::parseExpression(const char *terminators) {

   // check for a method
   Token tok = toker.getToken();
   if (tok.isIdent()) {

      // is it an assignment?
      Token tok1 = toker.getToken();
      if (tok1.isAssign()) {

	 // start the method with an augmented name XXX this business of
	 // passing a modified token is a hack.  Should really pass in the
	 // method name as a separate variable from the start token.
         // XXX write me

	 // parse an expression
	 if (!parseExpression(terminators)) {
	    tok1 = toker.getToken();
	    error(tok1, "expression expected");
	 }
      } else if (tok1.isLParen()) {
         // method invocation
	 FuncCallPtr funcCall =
            context->builder.createFuncCall(tok.getData());
	 parseMethodArgs(funcCall->args);
	 return ExprPtr::ucast(funcCall);
      } else {
         // for anything else, it's a variable
         toker.putBack(tok1);
         VarDefPtr def = context->lookUp(tok.getData());
         if (!def)
            error(tok,
                  SPUG_FSTR("Undefined variable: " << tok.getData()).c_str());

         // XXX def could be a generic class and generic classes require 
         // special magic to allow us to parse the <>'s
         VarDefPtr varDef = VarDefPtr::dcast(def);
         return ExprPtr::ucast(context->builder.createVarRef(varDef));
      }

      // close off the method no matter how we started it
      tok1 = toker.getToken();
      toker.putBack(tok1);
   } else if (tok.isString()) {
      return context->getStrConst(tok.getData());
   } else if (tok.isInteger()) {
      return context->builder.createIntConst(*context, 
                                             atoi(tok.getData().c_str())
                                             );
   } else if (tok.isLCurly()) {
      assert(false);
   } else {
      // not an expression
      toker.putBack(tok);
      return 0;
   }      // get the next token - if it is a dot, get a nested type


   // parse any following secondary expressions...
   tok = toker.getToken();
   while (true) {
      if (tok.isDot()) {
	 // get the next token, which should be an identifier
	 tok = toker.getToken();
	 if (!tok.isIdent())
	    error(tok, "identifier expected");

	 // process the method XXX need to add the "self" expression
	 FuncCallPtr funcCall = 
            context->builder.createFuncCall(tok.getData());
	 parseMethodArgs(funcCall->args);

      } else if (isBinaryOperator(tok)) {
         assert(false); // XXX lookup the binary expression and create a 
                        // function call
	 //return parseExpression();
      } else {
	 // next token is not part of the expression
	 break;
      }

      // get the next token
      tok = toker.getToken();

   }
   toker.putBack(tok);

   // Failed to parse an expression XXX need to deal with binary expressions 
   // correctly.
   return ExprPtr(0);
}

// func( arg, arg)
//      ^         ^
void Parser::parseMethodArgs(FuncCall::ExprVector &args) {
     
   while (true) {
      // try to parse an expression, if we failed we're done
      ExprPtr arg = parseExpression(",)");
      if (!arg)
	 break;
      args.push_back(arg);

      // comma signals another argument
      Token tok = toker.getToken();
      if (tok.isComma())
	 continue;

      // check for a right paren or semicolon, depending on if we got opening 
      // parens
      if (!tok.isRParen()) {
	 unexpected(tok, "expected closing paren or comma in argument list");
      } else {
	 break;
      }
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
         // XXX need function definitions/declarations
         assert(false);
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

void Parser::parseIfClause() {
   Token tok = toker.getToken();
   if (tok.isLCurly()) {
      parseBlock(true);
   } else {
      toker.putBack(tok);
      ExprPtr expr = parseExpression("; ");
      if (!expr)
         unexpected(tok, "expected expression.");
      expr->emit(*context);
      tok = toker.getToken();
      // XXX need to accept end of stream or '}', depending on the context
      if (!tok.isSemi())
         unexpected(tok, "expected semicolon.");
   }
}
   
// clause := expr ;   (';' can be replaced with EOF)
//        |  { block }
// if ( expr ) clause
//   ^               ^
// if ( expr ) clause else clause
//   ^                           ^
void Parser::parseIfStmt() {
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

   parseIfClause();

   // check for the "else"
   tok = toker.getToken();
   if (tok.isElse()) {
      pos = context->builder.emitElse(*context, pos);
      parseIfClause();
      context->builder.emitEndIf(*context, pos);
   } else {
      toker.putBack(tok);
      context->builder.emitEndIf(*context, pos);
   }
}

// while ( expr ) stmt ; (';' can be replaced with EOF)
//      ^               ^
// while ( expr ) { ... }
//      ^                ^
void Parser::parseWhileStmt() {
   Token tok = toker.getToken();
   if (!tok.isLParen())
      unexpected(tok, "expected left paren after while");
   
   ExprPtr expr = parseExpression(")");
   tok = toker.getToken();
   if (!tok.isRParen())
      unexpected(tok, "expected right paren after conditional expression");
   
   BranchpointPtr pos = context->builder.emitWhile(*context, expr);
   parseIfClause();
   context->builder.emitEndWhile(*context, pos);
}

void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false);
}

void Parser::error(const Token &tok, const char *msg) {
   Location loc = tok.getLocation();
   stringstream text;
   text << loc.getName() << ':' << loc.getLineNumber() << ": " << msg;
   throw ParseError(text.str().c_str());
}

void Parser::warn(const Token &tok, const std::string & msg) {
   Location loc = tok.getLocation();
   cerr << loc.getName() << ":" << loc.getLineNumber() << ": " << msg << endl;
}
