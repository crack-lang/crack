// Copyright 2009-2012 Google Inc.
// Copyright 2010,2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Arno Rehn <arno@arnorehn.de>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <spug/Exception.h>

#include "Toker.h"
#include "model/FuncCall.h"
#include "model/GenericParm.h"
#include "model/Context.h"
#include "model/ContextStackFrame.h"

namespace model {
   SPUG_RCPTR(ArgDef);
   SPUG_RCPTR(FuncDef);
   SPUG_RCPTR(Expr);
   class Generic;
   SPUG_RCPTR(Initializers);
   class Namespace;
   SPUG_RCPTR(TypeDef);
   SPUG_RCPTR(VarDef);
};

namespace parser {

class Parser;

// Callback interface.
struct ParserCallback {
   virtual void run(parser::Parser *parser, Toker *toker,
                    model::Context *context
                    ) = 0;
};

class Parser {

   public:

      // parse events that can be tied to a callback
      enum Event {
         funcDef, // after open paren for args in a functino def.
         funcEnter, // after first curly brace of the function block.
         funcLeave, // before the last curly brace of the function block.
         funcForward, // right after a forward declaration
         classDef, // after "class" keyword in a class def
         classEnter, // after opening brace at the beginning of a class def.
         classLeave, // before closing brace at the end of a class def.
         variableDef, // after the semicolon, assignment or comma of a variable
                      // definition.
         exprBegin, // beginning of an expression.
         controlStmt, // after the starting keyword of a control statement
                      // (including "import" and the empty statement)
         noCallbacks, // special event that you can't add callbacks to.
         eventSentinel // MUST BE THE LAST SYMBOL IN THE ENUM.
      };

   private:

      enum { noPrec, logOrPrec, logAndPrec, bitOrPrec, bitXorPrec,
             bitAndPrec, cmpPrec, shiftPrec, addPrec, multPrec, unaryPrec
            };

      Toker &toker;

      // often during a parse error, we want to point the user to the location
      // that wasn't the last source token location, but instead to e.g. an
      // appropriate identifier. in these cases, we save that location here.
      Location identLoc;

      // the module context, and the current context.
      model::ContextPtr moduleCtx, context;

      // sequential identifier used in nested block namespaces
      int nestID;

      /**
       * This class essentially lets us manage the context stack with the
       * program's stack.  We push the context by creating an instance, and
       * pop it by calling restore() or falling through to the destructor.
       */
      friend class model::ContextStackFrame<Parser>;

      typedef std::map<std::string, unsigned> OpPrecMap;
      OpPrecMap opPrecMap;

      // callbacks for each event type.
      typedef std::vector<ParserCallback *> CallbackVec;
      CallbackVec callbacks[eventSentinel];

      /**
       * Add a new definition to the current context or nearest definition
       * context.
       */
      void addDef(model::VarDef *context);

      void addFuncDef(model::FuncDef *funcDef);

      /**
       * Returns the next token from the tokenizer and stores its location in
       * the current context.
       */
      Token getToken();

      /**
       * Returns the precedence of the specified operator.
       */
      unsigned getPrecedence(const std::string &op);

      /** Special kind of error function used for unexpected tokens. */
      void unexpected(const Token &tok, const char *userMsg = 0);

      /**
       * Get the next token and make sure it is of type 'type' otherwise
       * unexpected(tok, error);
       */
      void expectToken(Token::Type type, const char *error);

      /** Look up a binary operator, either as a stand-alone operator
       * defintiion with two args or a method with one arg.
       *
       * @param name the operator name (e.g. "oper +")
       * @param args a two element arg list, lhs and rhs values.  This will be
       *    modified on return to include the correct argument list for the
       *    function or method.
       * @returns the function, null if there was no match.
       */
      model::FuncDefPtr lookUpBinOp(const std::string &name,
                                    model::FuncCall::ExprVec &args
                                    );

      /**
       * Make an assignment expression from the lvalue, operation token and
       * rvalue.
       */
      model::ExprPtr makeAssign(model::Expr *lvalue, const Token &tok,
                                model::Expr *rvalue
                                );

      /**
       * Parse an annotation and execute it.
       */
      void parseAnnotation();

      /**
       * Parse a single statement.
       * @param defsAllowed true if a definition may be provided instead of a
       *    statement.  Statements in block contexts may be definitions,
       *    simple statements in conditionals must not be.
       * @returns the context that this statement terminates to.  If non-null,
       *    the statement is terminal in all contexts from the current context
       *    to the returned context (non-inclusive).
       */
      model::ContextPtr parseStatement(bool defsAllowed);

      /**
       * Parse a block - a sequence of statements in the same execution
       * context.  A block is "nested" if it is inside the implicit file scope
       * block and wrapped in curly brackets.
       * @returns the context that this statement terminates to.  See
       *    parseStatement().
       */
      model::ContextPtr parseBlock(bool nested, Event closeEvent);

      /**
       * Creates a variable reference complete with an implicit "this" if
       * necessary.
       */
      model::ExprPtr createVarRef(model::Expr *receiver, model::VarDef *var,
                                  const Token &tok);

      /**
       * Create an assignment expression, complete with an implicit "this" if
       * necessary.
       */
      model::ExprPtr createAssign(model::Expr *container, const Token &ident,
                                  model::VarDef *var,
                                  model::Expr *val
                                  );
      /**
       * Create a variable reference expression for the identifier, complete
       * with an implicit "this" if necessary.
       * @param undefinedError If this is not null, it is an alternate error
       *    to use if the variable is undefined and it also makes the function
       *    return null rather than raising a parse error if the variable is
       *    an OverloadDef with multiple variations.
       */
      model::ExprPtr createVarRef(model::Expr *receiver, const Token &ident,
                                  const char *undefinedError = 0
                                  );

      /**
       * Parse the rest of an explicit "oper" name.
       */
      std::string parseOperSpec();

      /**
       * Parse an interpolated string.
       *
       * @param expr the receiver of the interpolated string operation.
       */
      model::ExprPtr parseIString(model::Expr *expr);

      /**
       * Parse a sequence constant.
       *
       * @param containerType the type of the sequence that we are
       * initializing.
       */
      model::ExprPtr parseConstSequence(model::TypeDef *containerType);

      /**
       * Parse a "typeof" expression.
       */
      model::TypeDefPtr parseTypeof();

      /**
       * If the expression is a VarRef referencing a TypeDef, return the
       * TypeDef.  Otherwise returns null.
       */
      static model::TypeDef *convertTypeRef(model::Expr *expr);

      /**
       * Parse the ternary operator's expression.
       * @param cond the conditional expression.
       */
      model::ExprPtr parseTernary(model::Expr *cond);

      struct Primary {
         model::ExprPtr expr;
         model::TypeDefPtr type;

         // 'ident' is defined if the primary consists only of a single
         // identifier.
         Token ident;

         Primary() {}
         Primary(model::Expr *expr) : expr(expr) {}
         Primary(model::Expr *expr, model::TypeDef *type, const Token &ident) :
            expr(expr),
            type(type),
            ident(ident) {
         }
      };

      /**
       * Emit (and return) an "oper class" call for the given expression.
       * 'tok' is the "class" token.
       */
      model::ExprPtr emitOperClass(model::Expr *expr, const Token &tok);

      /**
       * Raise a redefine error if we can't define a variable of the same name
       * as 'def'.
       */
      void checkForRedefine(const Token &tok, model::VarDef *def) const;

      /**
       * Parse a secondary expression.  Secondary expressions include a the
       * dot operator, binary operators and the bracket operators and their
       * associated expressions.
       *
       * @param primary the primary bundle that this secondary expression
       *        modifies.
       * @param precedence the current level of operator precedence.
       */
      model::ExprPtr parseSecondary(const Primary &primary,
                                    unsigned precedence = 0
                                    );

      /**
       * Parse an expression.
       *
       * @param precedence The function will not parse an operator of lower
       *    precedence than this parameter.  So if we're parsing the right
       *    side of 'x * y', and the precedence of '*' is 10, and we encounter
       *    the sequence 'a + b' ('+' has precedence of 8), we'll stop parsing
       *    after the 'a'.
       */
      model::ExprPtr parseExpression(unsigned precedence = 0);
      void parseMethodArgs(std::vector<model::ExprPtr> &args,
                           Token::Type terminator = Token::rparen
                           );

      /**
       * Parse the "specializer" after a generic type name.
       * @param tok the left bracket token of the generic specifier.
       * @param generic defined when doing this within a generic definition.
       */
      model::TypeDefPtr parseSpecializer(const Token &tok,
                                         model::TypeDef *typeDef,
                                         model::Generic *generic = 0
                                         );


      model::ExprPtr parseConstructor(const Token &tok, model::TypeDef *type,
                                      Token::Type terminator
                                      );

      model::TypeDefPtr parseTypeSpec(const char *errorMsg = 0,
                                      model::Generic *generic = 0
                                      );
      void parseModuleName(std::vector<std::string> &moduleName);
      void parseArgDefs(std::vector<model::ArgDefPtr> &args, bool isMethod);

      enum FuncFlags {
         normal,           // normal function
         hasMemberInits,   // function with member/base class initializers
                           // ("oper new")
         hasMemberDels,    // function with member/base class destructors
                           // ("oper del")
         reverseOp         // reverse binary operator.
      };

      /**
       * Parse a function definition.
       * @param returnType function return type.
       * @param nameTok the last token parsed in the function name.
       * @param name the full (but unqualified) function name.
       * @param funcFlags flags defining special processing rules for the
       *    function (whether initializers or destructors need to be parsed
       *    and emitted).
       * @param expectedArgCount if > -1, this is the expected number of arguments.
       * @returns the number of actual arguments.
       */
      int parseFuncDef(model::TypeDef *returnType, const Token &nameTok,
                       const std::string &name,
                       FuncFlags funcFlags,
                       int expectedArgCount
                       );

      /**
       * Parse a variable definition initializer.  Deals with the special
       * syntax for constructors and sequence constants.
       */
      model::ExprPtr parseInitializer(model::TypeDef *type,
                                      const std::string &varName
                                      );

      /**
       * Parse an alias statement.
       */
      void parseAlias();

      /**
       * Parse a definition. Returns false if there was no definition.
       * This will always parse the type specializer if it exists, and will
       * update "type" to point to its specialization.
       * @param type the parsed type.
       */
      bool parseTypeSpecializationAndDef(model::TypeDefPtr &type);

      /**
       * Parse a definition, returns false if there was no definition.
       * @param type the definition type.
       */
      bool parseDef(model::TypeDef *type);

      /** Parse a constant definition. */
      void parseConstDef();

      /**
       * Parse the scoping operator, e.g. foo::bar::baz
       * @param ns the primary namespace object (currently always a type).
       * @param var set to the final scoped def (the def associated with
       *    foo::bar::baz)
       * @param lastName set to the last name segment ("baz").
       */
      bool parseScoping(model::Namespace *ns, model::VarDefPtr &var,
                        Token &lastName
                        );

      // statements

      /**
       * Parse a clause, which can either be a an expression statement or a
       * definition.
       */
      void parseClause(bool defsAllowed);

      /*
       * @returns the context that this statement terminates to.  See
       *    parseStatement().
       */
      model::ContextPtr parseIfClause();

      /** Parse the condition in a 'while', 'for' or 'if' statement */
      model::ExprPtr parseCondExpr();

      /*
       * @returns the context that this statement terminates to.  See
       *    parseStatement().
       */
      model::ContextPtr parseIfStmt();

      void parseWhileStmt();
      void parseForStmt();
      void parseReturnStmt();
      model::ContextPtr parseTryStmt();
      model::ContextPtr parseThrowStmt();

      /**
       * Parse an import statement.  'ns' is the namespace in which to alias
       * imported symbols.
       */
      model::ModuleDefPtr parseImportStmt(model::Namespace *ns,
                                          bool annotation);

      /**
       * Parse a function definition after an "oper" keyword.
       */
      void parsePostOper(model::TypeDef *returnType);

      /** Parse the generic parameter list */
      void parseGenericParms(model::GenericParmVec &parms);

      void recordIStr(model::Generic *generic);
      void recordBlock(model::Generic *generic);
      void recordParenthesized(model::Generic *generic);

      model::TypeDefPtr parseClassDef();

   public:

      // XXX should be protected, once required functionality is migrated out.
      // error checking functions
      model::VarDefPtr checkForExistingDef(const Token &tok,
                                           const std::string &name,
                                           bool overloadOk = false
                                           );

      // XXX should be protected, once required functionality is migrated out.
      // checks if "existingDef" is an OverloadDef that contains an overload
      // matching argDefs.  Raises an error if this is an illegal overload,
      // otherwise returns the function that we're overriding if the override
      // needs to be considered (for implementation of a virtual or forward
      // function).
      model::FuncDefPtr checkForOverride(model::VarDef *existingDef,
                                         const model::ArgVec &argDefs,
                                         model::Namespace *ownerNS,
                                         const Token &nameTok,
                                         const std::string &name
                                         );

      // current parser state.
      enum State { st_base, st_optElse, st_notBase } state;

      Parser(Toker &toker, model::Context *context);

      void parse();

      /**
       * Parse the initializer list after an oper init.
       */
      void parseInitializers(model::Initializers *inits, model::Expr *receiver);

      void parseClassBody();

      /**
       * Throws a parse error about an attempt to override a variable that has
       * been defined in the same context.
       * @param tok the last parsed token
       * @param existing the existing variable definition.
       */
      void redefineError(const Token &tok,
                         const model::VarDef *existing
                         ) const;

      /**
       * throws a ParseError, properly formatted with the location and
       * message text.
       */
      void error(const Token &tok, const std::string &msg) const;
      void error(const Location &loc, const std::string &msg) const;

      /** Writes a warning message to standard error. */
      void warn(const Location &loc, const std::string &msg) const;

      /** Writes a warning message to standard error. */
      void warn(const Token &tok, const std::string & msg) const;

      /**
       * Add a new callback to the parser.  It is the responsibility of the
       * caller to manage the lifecycle of the callback and to insure that it
       * remains in existence until it is removed with removeCallback().
       */
      void addCallback(Event event, ParserCallback *callback);

      /**
       * Remove an existing callback.  This removes only the first added
       * instance of the callback, so if the same callback has been added N
       * times, N - 1 instances will remain.
       * Returns true if the callback was removed, false if it was not found.
       */
      bool removeCallback(Event event, ParserCallback *callback);

      /**
       * Run all callbacks associated with the event.
       *
       * Returns true if any callbacks were called.
       */
      bool runCallbacks(Event event);

      /**
       * Parse the selector after a dot operator.
       * @param expr the receiver expression to the left of the dot.  This is
       *             updated with the new value of the expression after the
       *             .selector is parsed.
       * @param type If the receiver is a type, this is it.  This is updated
       *             to the new type if .selector is a type, to null if it is
       *             not.
       */
      void parsePostDot(model::ExprPtr &expr, model::TypeDefPtr &type);

      /** Parse a "primary" */
      Primary parsePrimary(model::Expr *implicitReceiver);

      model::ExprPtr parseDefine(const Token &ident);
};

} // namespace parser

#endif
