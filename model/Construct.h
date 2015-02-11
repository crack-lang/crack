// Copyright 2011-2012 Google Inc.
// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Construct_h_
#define _model_Construct_h_

#include <list>
#include <stack>
#include <sys/time.h>

#include "model/StrConst.h"
#include "ModuleDef.h"
#include "Options.h"

namespace builder {
    SPUG_RCPTR(Builder);
}

namespace crack { namespace ext {
    class Module;
}}

namespace model {

SPUG_RCPTR(ModuleDef);

SPUG_RCPTR(Construct);
SPUG_RCPTR(StrConst);
SPUG_RCPTR(ConstructStats);

class ConstructStats : public spug::RCBase {

public:
    enum CompileState { start=0, builtin, parser, builder, executor, end };
    typedef std::map<std::string, double> ModuleTiming;

protected:
    unsigned int parsedCount;
    unsigned int cachedCount;
    double timing[end+1];
    ModuleTiming parseTimes;
    ModuleTiming buildTimes;
    ModuleTiming executeTimes;
    struct timeval lastTime;
    model::ModuleDefPtr curModule;
    CompileState curState;

    void showModuleCounts(std::ostream &out,
                          const std::string &title,
                          const ModuleTiming &list) const;

    void stopwatch();

public:

    ConstructStats(void):
        curState(start),
        parsedCount(0),
        cachedCount(0) {

        gettimeofday(&lastTime, NULL);
        for (int i = start; i <= end; i++)
            timing[i] = 0.0;
    }

    void setModule(model::ModuleDef *m) { curModule = m; }
    model::ModuleDefPtr getModule() const { return curModule; }

    void setState(CompileState newState) { stopwatch(); curState = newState; }
    CompileState getState() const { return curState; }

    void incParsed() { parsedCount++; }
    void incCached() { cachedCount++; }

    void write(std::ostream &out) const;

};

/**
 * Construct is a bundle containing the builder and all of the modules created 
 * using the builder.  It serves as a module cache and a way of associated the 
 * cache with a Builder that can create new modules.
 * 
 * A crack executor will contain either one or two Constructs - there is one 
 * for the program being executed and there may be different one.
 */
class Construct : public spug::RCBase, public Options {

    public:
        typedef std::vector<std::string> StringVec;
        typedef StringVec::const_iterator StringVecIter;

        struct ModulePath {
            std::string base, relPath, path;
            bool found, isDir;
            
            ModulePath(const std::string &base, const std::string &relPath,
                       const std::string &path, 
                       bool found, 
                       bool isDir
                       ) :
                base(base),
                relPath(relPath),
                path(path),
                found(found),
                isDir(isDir) {
            }
        };

        typedef void (*CompileFunc)(crack::ext::Module *mod);
        typedef void (*InitFunc)();
    
    private:
        std::stack<builder::BuilderPtr> builderStack;

        // hook into the runtime module's uncaught exception function.        
        bool (*uncaughtExceptionFunc)();
        
        // TODO: refactor this out of Namespace
        typedef std::map<std::string, VarDefPtr> VarDefMap;
        
        // A global mapping of definitions stored by their canonical names.  
        // Use of the registry is optional.  It currently facilitates caching.
        VarDefMap registry;
        
        // pointer to the crackLang module, if we're bootstrapped
        ModuleDefPtr crackLang;

    public: // XXX should be private
        // if non-null, this is the alternate construct used for annotations.  
        // If it is null, either this _is_ the annotation construct or both 
        // the main program and its annotations use the same construct.
        ConstructPtr compileTimeConstruct;
    
        // the toplevel builder        
        builder::BuilderPtr rootBuilder;

        // the toplevel context
        ContextPtr rootContext;

        // .builtin module, containing primitive types and functions
        ModuleDefPtr builtinMod;

        // mapping from the canonical name of the module to the module 
        // definition.
        typedef std::map<std::string, ModuleDefPtr> ModuleMap;
        ModuleMap moduleCache;

        // list of all modules in the order that they were loaded.
        std::vector<ModuleDefPtr> loadedModules;

        // the library path for source files.
        std::vector<std::string> sourceLibPath;

        // if we keep statistics, they reside here
        ConstructStatsPtr stats;

        // Trace caching decisions.
        static bool traceCaching;

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
                                     const std::string &extension,
                                     int verbosity=0
                                     );

        /**
         * Search the specified path for the 'relPath'.
         */
        static ModulePath searchPath(const Construct::StringVec &path,
                                     const std::string &relPath,
                                     int verbosity = 0
                                     );

        /**
         * Search the module source path for the specified file.  If the file 
         * is an absolute path, checks for an absolute path of that name.
         */
        ModulePath searchSourcePath(const std::string &path) const;

        /**
         * Returns true if 'name' is a valid file.
         */
        static bool isFile(const std::string &name);

        /**
         * Returns true if 'name' is a valid directory.
         */
        static bool isDir(const std::string &name);
        
        /**
         * Join a file name from a base directory and a relative path.
         */
        static std::string joinName(const std::string &base,
                                    const std::string &rel
                                    );

        /**
         * Join a file name from a pair of iterators and an extension.
         */
        static std::string joinName(StringVecIter pathBegin,
                                    StringVecIter pathEnd,
                                    const std::string &ext
                                    );

        /**
         * Creates a root context for the construct.
         */
        ContextPtr createRootContext();
        
    public:        

        // the error context stack.  This needs to be global because it is 
        // managed by annotations and transcends local contexts.
        std::list<std::string> errorContexts;
        
        // global string constants
        typedef std::map<std::string, StrConstPtr> StrConstTable;
        StrConstTable strConstTable;
        
        // built-in types.
        TypeDefPtr classType,
                   voidType,
                   voidptrType,
                   boolType,
                   byteptrType,
                   byteType,
                   int16Type,
                   int32Type,
                   int64Type,
                   uint16Type,
                   uint32Type,
                   uint64Type,
                   intType,
                   uintType,
                   intzType,
                   uintzType,
                   float32Type,
                   float64Type,
                   floatType,
                   vtableBaseType,
                   objectType,
                   stringType,
                   staticStringType,
                   overloadType,
                   crackContext,
                   arrayType,
                   functionType;

        // Size of these PDNTs in bits.
        int intSize, intzSize;

        Construct(const model::Options &options, builder::Builder *rootBuilder, 
                  Construct *primary = 0
                  );

        /**
         * Adds the given path to the source library path - 'path' is a 
         * colon separated list of directories.
         */
        void addToSourceLibPath(const std::string &path);

        /**
         * Loads the built-in modules (should be called prior to attempting to 
         * load or run anything else).
         */
        void loadBuiltinModules();

        /**
         * Parse the specified module out of the input stream.  Raises all 
         * ParseError's that occur.
         */
        void parseModule(Context &moduleContext,
                         ModuleDef *module,
                         const std::string &path,
                         std::istream &src
                         );

        /**
         * Initialize an extension module.  This only needs to be called for
         * the internal extension modules - ones that are bundled with the 
         * crack compiler shared library.
         */
        ModuleDefPtr initExtensionModule(const std::string &canonicalName,
                                         CompileFunc compileFunc,
                                         InitFunc initFunc
                                         );

        /**
         * Load a shared library.  Should conform to the crack extension 
         * protocol and implement a module init method.
         */
        ModuleDefPtr loadSharedLib(const std::string &path,
                                   StringVecIter moduleNameBegin,
                                   StringVecIter moduleNameEnd,
                                   std::string &canonicalName
                                   );

        /**
         * Try to get the module but only from the in-memory and persistent 
         * cache (the persistent cache only if caching is enabled and 
         * the module is cached).  Will not attempt to compile the module like 
         * getModule().  Returns the module if it is available, null 
         * if not.
         */
        ModuleDefPtr getCachedModule(const std::string &canonicalName);

        /**
         * Get the module wither from the in-memory cache, the persistent 
         * cache, or finally by compiling it from source.
         */
        ModuleDefPtr getModule(const std::string &canonicalName);

        /**
         * Load the named module and returns it.  Returns null if the module 
         * could not be found, raises an exception if there were errors 
         * parsing the module.
         */
        ModuleDefPtr getModule(StringVecIter moduleNameBegin,
                               StringVecIter moduleNameEnd,
                               std::string &canonicalName
                               );

        /**
         * Load the executor's bootstrapping modules (crack.lang).
         */
        bool loadBootstrapModules();

        /** 
         * Register a module with the module cache and the loaded module list. 
         *  This is intended to accomodate ephemeral modules
         */
        void registerModule(ModuleDef *module);

        /**
         * Run the specified script.  Catches all parse exceptions, returns 
         * an exit code, which will be non-zero if a parse error occurred and 
         * should eventually be settable by the application.
         * 
         * @param src the script's source stream.
         * @param name the script's name (for use in error reporting and 
         *  script module creation).
         * @param notAFile Set to true if the input is not a file and 
         *  therefore should not be cached (added for scripts read from 
         *  standard input).
         */
        int runScript(std::istream &src, const std::string &name,
                      bool notAFile
                      );

        /**
         * Returns the current builder.
         */        
        builder::Builder &getCurBuilder();
        
        /**
         * Register the definition in the global registry, storing it by its 
         * canonical name.  You must be able to call getFullName() on def to 
         * retrieve the canonical name, which generally means that the 
         * definition must have an owner.
         */
        void registerDef(VarDef *def);
        
        /**
         * Returns the definition registered with registerDef(), or null if 
         * no definition with the name was ever registered.
         */
        VarDefPtr getRegisteredDef(const std::string &canonicalName);
};

} // namespace model

#endif

