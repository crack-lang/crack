
#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <spug/Exception.h>

#include "Toker.h"

namespace model {
   SPUG_RCPTR(ArgDef);
   SPUG_RCPTR(Context);
   SPUG_RCPTR(Expr);
   SPUG_RCPTR(TypeDef);
};

namespace parser {

class Parser {

   private:

      Toker &toker;
      model::ContextPtr context;
      
      void unexpected(const Token &tok, const char *userMsg = 0);

      /**
       * Parse a single statement.
       * @param defsAllowed true if a definition may be provided instead of a 
       *    statement.  Statements in block contexts may be definitions, 
       *    simple statements in conditionals must not be.
       * @returns true if the statement is terminal (always returns or raises 
       *    an exception).
       */
      bool parseStatement(bool defsAllowed);

      /**
       * Parse a block - a sequence of statements in the same execution
       * context.  A block is "nested" if it is inside the implicit file scope
       * block and wrapped in curly brackets.
       * @returns true if the statement is terminal (always returns or raises 
       *    an exception).
       */
      bool parseBlock(bool nested);

      /** returns true if the token is a binary operator. */
      static bool isBinaryOperator(const Token &tok);
      
      /** Create a reference to the "this" variable, error if there is none. */
      model::ExprPtr makeThisRef(const Token &ident);

      /**
       * Parse the kinds of things that can come after an identifier.
       *
       * @param container the aggregate that the identifier is scoped to, as 
       *   in "container.ident"  This can be null, in which case the 
       *   identifier is scoped to the local context.
       * @param ident The identifier's token.
       */
      model::ExprPtr parsePostIdent(const model::ExprPtr &container,
                                    const Token &ident
                                    );

      /**
       * @param terminators a list of termination symbols that are appropriate 
       *   for the expression (example ",)" for a parameter expression) .  A 
       *   space (ascii 32) indicates that the end-of-stream token is a
       *   terminator.
       */
      model::ExprPtr parseExpression();
      void parseMethodArgs(std::vector<model::ExprPtr> &args);

      model::TypeDefPtr parseTypeSpec();
      void parseArgDefs(std::vector<model::ArgDefPtr> &args);

      /** Parse a definition. Returns false if there was no definition. 
       * @param type the parsed type.
       */
      bool parseDef(const model::TypeDefPtr &type);
      
      // statements
      
      bool parseIfClause();
      bool parseIfStmt();
      bool parseWhileStmt();
      void parseReturnStmt();
      model::TypeDefPtr parseClassDef();
      
      // context stack manipulation
      void pushContext(const model::ContextPtr &newContext);
      void popContext();

      // error checking functions
      void checkForExistingDef(const Token &tok);

   public:
      Parser(Toker &toker, const model::ContextPtr &context) : 
	 toker(toker),
	 context(context) {
      }

      void parse();

      /** 
       * throws a ParseError, properly formatted with the location and
       * message text.
       */
      static void error(const Token &tok, const std::string &msg);
      
      /** Writes a warning message to standard error. */
      static void warn(const Token &tok, const std::string & msg);
};

} // namespace parser

#endif
