// Copyright 2010 Google Inc.

#ifndef _Crack_h_
#define _Crack_h_

#include "builder/BuilderOptions.h"

#include <map>
#include <vector>
#include <spug/RCPtr.h>

namespace model {
    SPUG_RCPTR(Construct);
    SPUG_RCPTR(Context);
    SPUG_RCPTR(ModuleDef);
}

namespace builder {
    SPUG_RCPTR(Builder);
}

/**
 * High-level wrapper around the crack executor.  Use this whenever possible 
 * for embedding.
 */
class Crack {
    private:
        // the root context contains all of the builtin types and functions
        // that are visible from all modules.
        model::ContextPtr rootContext;
        
        // the primary construct.
        model::ConstructPtr construct;

        // keeps init() from doing its setup stuff twice.
        bool initialized;

        bool init();

    public:

        // builder specific options
        builder::BuilderOptionsPtr options;

        // if true, don not load the bootstrapping modules before running a 
        // script.  This changes some of the language semantics: constant 
        // strings become byteptr's and classes without explicit ancestors
        // will not be derived from object.
        bool noBootstrap;
        
        // if true, add the global installed libary path to the library search 
        // path prior to running anything.
        bool useGlobalLibs;
        
        // if true, emit warnings when the code has elements with semantic
        // differences from the last version of the language.
        bool emitMigrationWarnings;
        
        Crack(void);
        
//        ~Crack();
        
    public:         

        /**
         * Adds the given path to the source library path - 'path' is a 
         * colon separated list of directories.
         */
        void addToSourceLibPath(const std::string &path);

        /**
         * Set the program's arg list (this should be done prior to calling 
         * runScript()).
         */
        void setArgv(int argc, char **argv);

        /**
         * set the main builder for compiling runtime code
         */
        void setBuilder(builder::Builder *builder);

        /**
         * Set the builder to be used by the compiler for annotation modules.
         */
        void setCompileTimeBuilder(builder::Builder *builder);

        /**
         * Run the specified script.  Catches all parse exceptions, returns 
         * an exit code, which will be non-zero if a parse error occurred and 
         * should eventually be settable by the application.
         * 
         * @param src the script's source stream.
         * @param name the script's name (for use in error reporting and 
         *  script module creation).
         */
        int runScript(std::istream &src, const std::string &name);

        /**
         * Call the module destructors for all loaded modules in the reverse 
         * order that they were loaded.  This should be done before
         * terminating.
         */
        void callModuleDestructors();        
};

#endif

