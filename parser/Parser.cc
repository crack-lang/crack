
#include "Parser.h"

#include <sstream>
#include <stdexcept>
#include <spug/StringFmt.h>
#include "model/ArgDef.h"
#include "model/AssignExpr.h"
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

void Parser::addDef(const VarDefPtr &varDef) {
   context->getDefContext()->addDef(varDef);
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
      if (def && dynamic_cast<TypeDef *>(def.obj) &&
          parseDef(TypeDefPtr::dcast(def))
          ) {
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
            VarDefPtr varDef =
               expr->type->emitVarDef(*context, tok.getData(), expr);
            addDef(varDef);
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

namespace {
   // emit cleanups for the block unless there was a terminal statement
   inline bool emitCleanups(const ContextPtr &context,
                            bool gotTerminalStatement
                            ) {
      if (gotTerminalStatement)
         return true;
      
      context->emitCleanups(Context::block);
      return false;
   }
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
            return emitCleanups(context, gotTerminalStatement);
         else
            unexpected(tok, "expected statement or closing brace.");
      } else if (tok.isEnd()) {
         if (!nested)
            return emitCleanups(context, gotTerminalStatement);
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

ExprPtr Parser::makeThisRef(const Token &ident) {
   VarDefPtr thisVar = context->lookUp("this");
   if (!thisVar)
      error(ident,
            SPUG_FSTR("instance member " << ident.getData() <<
                       " may not be used in a static context."
                      )
            );
                      
   return ExprPtr::ucast(context->builder.createVarRef(thisVar));
}

ExprPtr Parser::parsePostIdent(const ExprPtr &container, const Token &ident) {
   Context &varContext = container ? *container->type->context : *context;
   // is it an assignment?
   Token tok1 = toker.getToken();
   if (tok1.isAssign()) {

      // look up the variable
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
         return AssignExpr::create(ident, receiver, var, val);
      } else {
         return AssignExpr::create(ident, var, val);
      }

   } else if (tok1.isLParen()) {
      // function/method invocation
      
      // parse the arg list
      FuncCall::ExprVector args;
      parseMethodArgs(args);
      
      // lookup the method from the variable context's type context
      // XXX needs to handle callable objects.
      FuncDefPtr func = varContext.lookUp(ident.getData(), args);
      if (!func)
         error(ident,
               SPUG_FSTR("No method exists matching " << ident.getData() <<
                          " with these argument types."
                         )
               );

      // if the definition is for an instance variable, emit an implicit 
      // "this" dereference.  Otherwise just emit the variable
      ExprPtr receiver;
      if (func->context->scope == Context::instance)
         // if there's no container, try to use an implicit "this"
         receiver = container ? container : makeThisRef(ident);

      FuncCallPtr funcCall = context->builder.createFuncCall(func);
      funcCall->args = args;
      funcCall->receiver = receiver;
      return ExprPtr::ucast(funcCall);
   } else {
      // for anything else, it's a variable reference
      toker.putBack(tok1);
      VarDefPtr var = varContext.lookUp(ident.getData());
      if (!var)
         error(ident,
               SPUG_FSTR("Undefined variable: " << ident.getData()));

      // if the definition is for an instance variable, emit an implicit 
      // "this" dereference.  Otherwise just emit the variable
      if (var->context->scope == Context::instance) {
         // if there's no container, try to use an implicit "this"
         ExprPtr receiver = container ? container : makeThisRef(ident);
         return context->builder.createFieldRef(receiver, var);
      } else {
         return ExprPtr::ucast(context->builder.createVarRef(var));
      }
   }

}

ExprPtr Parser::parseExpression() {

   ExprPtr expr;

   // check for a method
   Token tok = toker.getToken();
   if (tok.isIdent()) {
      expr = parsePostIdent(0, tok);
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

         expr = parsePostIdent(expr, tok);
      } else if (isBinaryOperator(tok)) {
         // parse the right-hand-side expression
         ExprPtr rhs = parseExpression();
         
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
      addDef(VarDefPtr::ucast(argDef));
      
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
         VarDefPtr varDef = type->emitVarDef(*context, varName, 0);
         addDef(varDef);
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
            initializer = parseExpression();
            
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
         addDef(varDef);
         return true;
      } else if (tok3.isLParen()) {
         // function definition

         // if this is a class context, we're defining a method.
         ContextPtr classCtx = context->getClassContext();
         bool isMethod = classCtx ? true : false;

         // push a new context, arg defs will be stored in the new context.
         ContextStackFrame cstack(*this,
                                  context->createSubContext(Context::local)
                                  );
         context->returnType = type;
         
         // if this is a method, add the "this" variable
         if (isMethod) {
            assert(classCtx && "method not in class context.");
            ArgDefPtr argDef =
               context->builder.createArgDef(classCtx->returnType, "this");
            addDef(argDef);
         }

         // parse the arguments
         vector<ArgDefPtr> argDefs;
         parseArgDefs(argDefs);
         
         tok3 = toker.getToken();
         if (!tok3.isLCurly())
            unexpected(tok3, "expected '{' in function definition");
         
         // parse the body
         FuncDef::Flags flags = isMethod ? FuncDef::method : FuncDef::noFlags;
         FuncDefPtr funcDef =
            context->builder.emitBeginFunc(*context, flags, varName, type,
                                           argDefs
                                           );
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
         cstack.restore();
         
         // store the new definition in the context.
         addDef(VarDefPtr::ucast(funcDef));
         
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
      ContextStackFrame cstack(*this, context->createSubContext());
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
   
   ExprPtr cond = parseExpression();
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
      pos = context->builder.emitElse(*context, pos, terminalIf);
      terminalElse = parseIfClause();
      context->builder.emitEndIf(*context, pos, terminalElse);
   } else {
      toker.putBack(tok);
      context->builder.emitEndIf(*context, pos, terminalIf);
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
   
   BranchpointPtr pos = context->builder.emitBeginWhile(*context, expr);
   bool terminal = parseIfClause();
   context->builder.emitEndWhile(*context, pos);
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
      context->emitCleanups(Context::function);
      context->builder.emitReturn(*context, 0);
      return;
   }

   // parse the return expression, make sure that it matches the return type.
   toker.putBack(tok);
   ExprPtr expr = parseExpression();
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
   
   // emit all of the cleanups for the function and the return
   context->emitCleanups(Context::function);
   context->builder.emitReturn(*context, expr);

   tok = toker.getToken();   
   if (tok.isEnd() || tok.isRCurly())
      toker.putBack(tok);
   else if (!tok.isSemi())
      unexpected(tok, "expected semicolon or block terminator");
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

   // store the base classes as the parent contexts of the class context
   for (vector<TypeDefPtr>::iterator iter = bases.begin();
        iter != bases.end();\
        ++iter
        )
       classContext->parents.push_back((*iter)->context);

   // push the context
   ContextStackFrame cstack(*this, lexicalContext);
   
   // emit the beginning of the class, hook it up to the class context and 
   // store a reference to it in the parent context.
   TypeDefPtr type = classContext->returnType =
      context->builder.emitBeginClass(*classContext, className, bases);
   type->context = classContext;
   cstack.parent().getDefContext()->addDef(type);

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
            toker.putBack(tok);
            parseDef(newType);
         }
      }
      
      // parse some other kind of definition
      toker.putBack(tok);
      TypeDefPtr type = parseTypeSpec();
      parseDef(type);
   }

   classContext->complete = true;
   classContext->builder.emitEndClass(*classContext);
   cstack.restore();
   
   return type;
}
   
void Parser::parse() {
   // outer parser just parses an un-nested block
   parseBlock(false);
}

void Parser::checkForExistingDef(const Token &tok) {
   VarDefPtr existing = context->lookUp(tok.getData());
   if (existing)
      // redefinition in the same context is an error, redefinition in a 
      // nested context is merely a warning.
      if (existing->context == context)
         error(tok, 
               SPUG_FSTR("Symbol " << tok.getData() <<
                          "is already defined in this context."
                         )
               );
      else
         warn(tok,
              SPUG_FSTR("Symbol " << tok.getData() << 
                         "hides another definition in an enclosing context."
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
