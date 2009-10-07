
#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>
#include <vector>
#include <iostream>
#include <spug/Exception.h>

#include "Toker.h"

namespace model {
   SPUG_RCPTR(Context);
   SPUG_RCPTR(Expr);
};

namespace parser {

class Parser {

   private:

      Toker &toker;
      model::ContextPtr context;
      
      void unexpected(const Token &tok, const char *userMsg = 0);

      /**
       * Parse a block - a sequence of statements in the same execution
       * context.  A block is "nested" if it is inside the implicit file scope
       * block and wrapped in curly brackets.
       */
      void parseBlock(bool nested);

      /** returns true if the token is a binary operator. */
      static bool isBinaryOperator(const Token &tok);
      model::ExprPtr parseExpression();
      void parseMethodArgs(std::vector<model::ExprPtr> &args);
      void parseVarDef(const Token &tok);

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
      static void error(const Token &tok, const char *msg);
};

} // namespace parser
#endif

