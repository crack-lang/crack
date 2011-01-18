// Copyright 2010 Google Inc.

#ifndef _Crack_h_
#define _Crack_h_

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

namespace crack { namespace ext {
    class Module;
}}

/**
 * High-level wrapper around the crack executor.  Use this whenever possible 
 * for embedding.
 */
class Crack {
    private:
        static Crack *theInstance;

        // the root context contains all of the builtin types and functions
        // that are visible from all modules.
        model::ContextPtr rootContext;
        
        // the primary construct.
        model::ConstructPtr construct;

        // keeps init() from doing its setup stuff twice.
        bool initialized;

        bool init();

    public:
        typedef std::vector<std::string> StringVec;
        typedef StringVec::const_iterator StringVecIter;

        struct ModulePath {
            std::string path;
            bool found, isDir;
            
            ModulePath(const std::string &path, bool found, bool isDir) :
                path(path),
                found(found),
                isDir(isDir) {
            }
        };
        
        // the library path for source files.
        std::vector<std::string> sourceLibPath;
        
        // if true, do not execute anything, but rather dump modules.
        bool dump;

        // optimization level to pass to builder
        int optimizeLevel;

        // if true, generate debug information suitable for a debugger
        bool emitDebugInfo;

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

        typedef void (*InitFunc)(crack::ext::Module *mod);

        
        Crack(builder::Builder *rootBuilder);
        
//        ~Crack();
        
        /**
         * Join 'base', all of the strings in 'path' and ext into a full path 
         * name (e.g. "base/p/a/t/h.ext").
         */
        static std::string joinName(const std::string &base,
                                    StringVecIter pathBegin,
                                    StringVecIter pathEnd,
                                    const std::string &ext
                                    );
        
        /**
         * Returns true if 'name' is a valid file.
         */
        static bool isFile(const std::string &name);

        /**
         * Returns true if 'name' is a valid directory.
         */
        static bool isDir(const std::string &name);
        
        /**
         * Search the specified path for a file with the name 
         * "moduleName.extension", if this does not exist, may also return the 
         * path for a directory named "moduleName."
         * 
         * @param path the list of root directories to search through.
         * @param moduleNameBegin the beginning of a vector of name 
         *  components to search for. Name components are joined together to 
         *  form a path, so for example the vector ["foo", "bar", "baz"] would 
         *  match files and directories with the path "foo/bar/baz" relative 
         *  to the root directory.
         * @paramModuleNameEnd the end of the name component vector.
         * @param extension The file extension to search for.  This is not 
         *  applied when matching a directory.
         */
        static ModulePath searchPath(const StringVec &path, 
                                     StringVecIter moduleNameBegin,
                                     StringVecIter modulePathEnd,
                                     const std::string &extension
                                     );

    public:

        /**
         * Singleton interface for Crack - this is just temporary until we 
         * fully migrate everything into Construct, then this won't be needed.
         */        
        static Crack &getInstance();
        static void setInstance(Crack &instance);

        /**
         * Adds the given path to the source library path - 'path' is a 
         * colon separated list of directories.
         */
        void addToSourceLibPath(const std::string &path);

        /**
         * Parse the specified module out of the input stream.  Raises all 
         * ParseError's that occur.
         */
        void parseModule(model::Context &moduleContext,
                         model::ModuleDef *module,
                         const std::string &path,
                         std::istream &src
                         );

        /**
         * Initialize an extension module.  This only needs to be called for
         * the internal extension modules - ones that are bundled with the 
         * crack compiler shared library.
         */
        model::ModuleDefPtr
            initExtensionModule(const std::string &canonicalName,
                                InitFunc initFunc
                                );

        /**
         * Load a shared library.  Should conform to the crack extension 
         * protocol and implement a module init method.
         */
        model::ModuleDefPtr
            loadSharedLib(const std::string &path,
                          StringVecIter moduleNameBegin,
                          StringVecIter moduleNameEnd,
                          std::string &canonicalName
                          );

        /**
         * Load the named module and returns it.  Returns null if the module 
         * could not be found, raises an exception if there were errors 
         * parsing the module.
         */
        model::ModuleDefPtr
            loadModule(StringVecIter moduleNameBegin,
                       StringVecIter moduleNameEnd,
                       std::string &canonicalName
                       );

        /**
         * Load the executor's bootstrapping modules (crack.lang).
         */
        bool loadBootstrapModules();

        /**
         * Set the program's arg list (this should be done prior to calling 
         * runScript()).
         */
        void setArgv(int argc, char **argv);


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
         * Load the named module and execute its toplevel code.
         */
        static model::ModuleDefPtr
            loadModule(const std::vector<std::string> &moduleName,
                       std::string &canonicalName
                       );
        
        /**
         * Call the module destructors for all loaded modules in the reverse 
         * order that they were loaded.  This should be done before
         * terminating.
         */
        void callModuleDestructors();
};

#endif

