
#ifndef PARSER_H
#define PARSER_H

#include <list>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <spug/Exception.h>

#include "Toker.h"

namespace model {
   SPUG_RCPTR(ArgDef);
   SPUG_RCPTR(Context);
   SPUG_RCPTR(Expr);
   SPUG_RCPTR(TypeDef);
   SPUG_RCPTR(VarDef);
};

namespace parser {

class Parser {

   private:

      Toker &toker;
      
      // the module context, and the current context.
      model::ContextPtr moduleCtx, context;
      
      /**
       * This class essentially lets us manage the context stack with the 
       * program's stack.  We push the context by creating an instance, and 
       * pop it by calling restore() or falling through to the destructor.
       */
      friend class ContextStackFrame;
      class ContextStackFrame {
         private:
            bool restored;
            Parser &parser;
            model::ContextPtr context;

         public:
            ContextStackFrame(Parser &parser,
                              model::Context *context
                              ) :
               restored(false),
               parser(parser),
               context(parser.context) {
               
               parser.context = context;
            }
            
            ~ContextStackFrame() {
               if (!restored)
                  restore();
            }
            
            void restore() {
               assert(!restored);
               parser.context = context;
               restored = true;
            }
            
            model::Context &parent() {
               assert(!restored);
               return *context;
            }
      };
      
      typedef std::map<std::string, unsigned> OpPrecMap;
      OpPrecMap opPrecMap;
      
      /**
       * Add a new definition to the current context or nearest definition 
       * context.
       */
      void addDef(model::VarDef *context);
      
      /**
       * Returns the precedence of the specified operator.
       */
      unsigned getPrecedence(const std::string &op);

      /** Special kind of error function used for unexpected tokens. */
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
      model::ExprPtr parsePostIdent(model::Expr *container,
                                    const Token &ident
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
      void parseMethodArgs(std::vector<model::ExprPtr> &args);

      model::TypeDefPtr parseTypeSpec();
      void parseModuleName(std::vector<std::string> &moduleName);
      void parseArgDefs(std::vector<model::ArgDefPtr> &args);

      /**
       * Parse a definition. Returns false if there was no definition. 
       * @param type the parsed type.
       */
      bool parseDef(model::TypeDef *type);
      
      // statements
      
      bool parseIfClause();
      bool parseIfStmt();
      bool parseWhileStmt();
      void parseReturnStmt();
      void parseImportStmt();
      model::TypeDefPtr parseClassDef();
      
      // error checking functions
      model::VarDefPtr checkForExistingDef(const Token &tok,
                                           bool overloadOk = false);

   public:
      Parser(Toker &toker, model::Context *context);

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
