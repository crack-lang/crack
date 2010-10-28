// Copyright 2010 Google Inc.

#ifndef _crack_compiler_CrackContext_h_
#define _crack_compiler_CrackContext_h_

namespace parser {
    class Parser;
    class Toker;
}

namespace model {
    class Context;
}

namespace compiler {

class Token;

/**
 * CrackContext is the argument for all annotations.  It gives an annotation 
 * access to the internals of the compiler and parser.
 */
class CrackContext {
    private:
        parser::Parser *parser;
        parser::Toker *toker;
        model::Context *context;

    public:
        CrackContext(parser::Parser *parser, parser::Toker *toker,
                     model::Context *context
                     );

        /**
         * Inject a null terminated string into the tokenizer.
         * The string will be tokenized and the tokens inserted into the 
         * stream before any existing tokens that have been pushed back.
         */
        void inject(char *code);        
        
        /**
         * Returns the next token from the tokenizer.
         */
        Token *getToken();
        
        /**
         * Put the token back into the tokenizer - it will again be the next 
         * token returned by getToken().
         */
        void putBack(Token *tok);
        
        /**
         * Returns the context scope.
         */
        int getScope();
};

} // namespace compiler

#endif
