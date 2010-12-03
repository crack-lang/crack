// Copyright 2010 Google Inc.

#ifndef _crack_compiler_CrackContext_h_
#define _crack_compiler_CrackContext_h_

namespace parser {
    class Parser;
    class ParserCallback;
    class Toker;
}

namespace model {
    class Context;
}

namespace compiler {

class Annotation;
class Location;
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
        void *userData;

    public:
        enum Event { funcEnter, funcLeave };

        CrackContext(parser::Parser *parser, parser::Toker *toker,
                     model::Context *context,
                     void *userData = 0
                     );

        /**
         * Inject a null terminated string into the tokenizer.
         * The string will be tokenized and the tokens inserted into the 
         * stream before any existing tokens that have been pushed back.
         */
        void inject(char *sourceName, int lineNumber, char *code);
        
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
        
        typedef void (*AnnotationFunc)(CrackContext *);

        /**
         * Stores a simple annotation function in the module context.
         */
        void storeAnnotation(const char *name, AnnotationFunc func);
        
        /**
         * Stores an annotation and user data in the module context.
         */
        void storeAnnotation(const char *name, AnnotationFunc func,
                             void *userData
                             );
        
        /**
         * Returns the named annotation, null if not found.
         */
        Annotation *getAnnotation(const char *name);
        
        /**
         * Returns the user data associated with the annotation.  User data 
         * can be stored in some types of Annotation objects and passed into 
         * the CrackContext when it is created.
         */
        void *getUserData();
        
        /**
         * Generate a compiler error, use the location of the last token as 
         * the error location.
         */
        void error(const char *text);
        
        /**
         * Generate a compiler error, use the location of the token as the 
         * error location.
         */
        void error(Token *tok, const char *text);

        /**
         * Generate a compiler warning, use the location of the last token as 
         * the error location.
         */
        void warn(const char *text);
        
        /**
         * Generate a compiler warning, use the location of the token as the 
         * error location.
         */
        void warn(Token *tok, const char *text);
        
        /**
         * Returns the state of the parser.
         */
        int getParseState();
        
        /**
         * Adds a callback for the specified event.  Returns the callback id.
         */
        parser::ParserCallback *addCallback(int event, AnnotationFunc func);
        
        /**
         * Remove the specified callback.  "id" is the value returned from 
         * addCallback().
         */
        void removeCallback(parser::ParserCallback *callback);
        
        /**
         * Set the flags for the next function.  Valid values are 
         * FUNCFLAG_STATIC and FUNCFLAG_FINAL.
         */
        void setNextFuncFlags(int nextFuncFlags);
        
        /** Create the specified location. */
        Location *getLocation(const char *name, int lineNumber);
        
        /** Returns the location of the last processed token. */
        Location *getLocation();
};

} // namespace compiler

#endif
