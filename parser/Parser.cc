
#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include "model/Context.h"
#include "model/Def.h"
#include "model/FuncCall.h"
#include "model/Expr.h"
#include "model/StrConst.h"
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

      if (tok.isIdent()) {
         
         // if the identifier is a type, check to see if the next identifier 
         // is another identifier.
//         if (context.lookUp(tok.
         
         // if the next token(s) are '=' or ':=' then this is an assignment
         
         toker.putBack(tok);

      } else {
	 toker.putBack(tok);
      }

      // parse an expression (if there is no expression, that's ok)
      ExprPtr expr = parseExpression();
      if (expr)
         // if there is an expression, emit it.
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

ExprPtr Parser::parseExpression() {

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
	 if (!parseExpression()) {
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
         error(tok1, "expected left paren or assignment operator.");
      }

      // close off the method no matter how we started it
      tok1 = toker.getToken();
      toker.putBack(tok1);
   } else if (tok.isString()) {
      return context->getStrConst(tok.getData());
   } else if (tok.isInteger()) {
      assert(false);
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
      ExprPtr arg = parseExpression();
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

void Parser::parseVarDef(const Token &ident) {
   Token tok;

   while (true) {

      // get a type identifier
      tok = toker.getToken();
      if (tok.isAssign()) {
         parseExpression();
         break;
      } else if (!tok.isIdent()) {
	 unexpected(tok, "expected identifier");
      }

      // get the next token, if it is an '=', parse an assignment expression, 
      // if it is a '.', parse a nested type.
      tok = toker.getToken();
      if (tok.isAssign()) {
         parseExpression();
         break;
      } else if (!tok.isDot()) {
	 toker.putBack(tok);
	 break;
      }
   }

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

