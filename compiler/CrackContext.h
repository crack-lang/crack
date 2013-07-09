// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_compiler_CrackContext_h_
#define _crack_compiler_CrackContext_h_

#include "init.h"

namespace crack { namespace ext {
    class Module;
}}

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
    friend void compiler::init(crack::ext::Module *mod);
    public:
        typedef void (*AnnotationFunc)(CrackContext *);
        
        struct AnnotationFunctor {
            virtual void run(CrackContext *context) = 0;
        };
        
        // Utility class, that allows us to do this:
        //    ctx->addCallback(new AnnotationFuncWrapper(&someFunction, obj))
        // someFunction should have the prototype:
        //      void someFunction(T *, CrackContext *);
        template <typename T>
        struct AnnotationFuncWrapper : public AnnotationFunctor {
            typedef void (T::*Func)(CrackContext *);
            Func func;
            T *obj;
            
            AnnotationFuncWrapper(Func func, T *obj) :
                func(func),
                obj(obj) {
            }
            
            virtual void run(CrackContext *context) {
                (obj->*func)(context);
            }
        };

    private:
        parser::Parser *parser;
        parser::Toker *toker;
        model::Context *context;
        void *userData;

        static void _inject(CrackContext *inst, char *sourceName, 
                            int lineNumber, 
                            char *code
                            );
        static Token *_getToken(CrackContext *inst);
        static void _putBack(CrackContext *inst, Token *tok);
        static int _getScope(CrackContext *inst);
        static void _storeAnnotation(CrackContext *inst, const char *name, 
                                     AnnotationFunc func
                                     );
        static void _storeAnnotation(CrackContext *inst, const char *name, 
                                     AnnotationFunc func,
                                     void *userData
                                     );
        static Annotation *_getAnnotation(CrackContext *inst, const char *name);
        static void *_getUserData(CrackContext *inst);
        static void _error(CrackContext *inst, const char *text);
        static void _error(CrackContext *inst, Token *tok, const char *text);
        static void _warn(CrackContext *inst, const char *text);
        static void _warn(CrackContext *inst, Token *tok, const char *text);
        static void _pushErrorContext(CrackContext *inst, const char *text);
        static void _popErrorContext(CrackContext *inst);
        static int _getParseState(CrackContext *inst);
        static parser::ParserCallback *_addCallback(CrackContext *inst, 
                                                    int event, 
                                                    AnnotationFunc func
                                                    );
        static void _removeCallback(CrackContext *inst, 
                                    parser::ParserCallback *callback
                                    );
        static void _setNextFuncFlags(CrackContext *inst, int nextFuncFlags);
        static unsigned int _getCurrentVTableOffset(CrackContext *inst);
        static Location *_getLocation(CrackContext *inst, const char *name, 
                                      int lineNumber
                                      );
        static Location *_getLocation(CrackContext *inst);
        static void _continueIString(CrackContext *inst);

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
        
        /**
         * Stores a simple annotation function in the context.
         */
        void storeAnnotation(const char *name, AnnotationFunc func);
        
        /**
         * Stores an annotation and user data in the context.
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
         * Push the string onto the context stack to be displayed for any 
         * error messages.
         */
        void pushErrorContext(const char *text);
        
        /**
         * Pop the last string off of the error message context stack.
         */
        void popErrorContext();
        
        /**
         * Returns the state of the parser.
         */
        int getParseState();
        
        /**
         * Adds a callback for the specified event.  Returns the callback id.
         */
        parser::ParserCallback *addCallback(int event, AnnotationFunc func);
        
        /**
         * Add a callback functor.  Returns a ParserCallback object that can 
         * be used with removeCallback().
         */
        parser::ParserCallback *addCallback(int event, 
                                            AnnotationFunctor *functor
                                            );
        
        /**
         * Remove the specified callback.  "id" is the value returned from 
         * addCallback().
         */
        void removeCallback(parser::ParserCallback *callback);
        
        /**
         * Set the flags for the next function.  Valid values are 
         * FUNCFLAG_STATIC, FUNCFLAG_FINAL and FUNCFLAG_ABSTRACT.
         */
        void setNextFuncFlags(int nextFuncFlags);
        
        /**
         * Get the offset in the virtual table of the current method.
         */
        unsigned int getCurrentVTableOffset() const;

        /**
         * Set the flags for the next class.  Valid values are 
         * CLASSFLAG_ABSTRACT.
         */
        void setNextClassFlags(int nextClassFlags);
        
        /** Create the specified location. */
        Location *getLocation(const char *name, int lineNumber);
        
        /** Returns the location of the last processed token. */
        Location *getLocation();
        
        /** Tells the tokenizer to resume parsing an i-string. */
        void continueIString();
        
        /**
         * This is a back-door for things like the compiler-defined "export" 
         * annotation to get access to the underlying context object without 
         * us having to wrap the entire data model.  It is not available to 
         * annotations written in crack.
         */
        model::Context *getContext() { return context; }
};

} // namespace compiler

#endif
