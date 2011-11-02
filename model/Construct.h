// Copyright 2010 Google Inc.

#ifndef _model_Construct_h_
#define _model_Construct_h_

#include "ModuleDef.h"
#include <list>
#include <stack>
#include <sys/time.h>

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

struct ConstructStats: public spug::RCBase {

    enum CompileState { start=0, builtin, build, run, end };

    typedef std::map<std::string, double> ModuleTiming;
    unsigned int parsedCount;
    unsigned int cachedCount;
    CompileState state;
    double timing[5];
    ModuleTiming moduleTimes;
    struct timeval lastTime;
    std::string currentModule;

    ConstructStats(void): parsedCount(0),
        cachedCount(0),
        state(start),
        timing(),
        moduleTimes(),
        lastTime(),
        currentModule("NONE") {
        gettimeofday(&lastTime, NULL);
        for (int i=0; i<5; i++)
            timing[i] = 0.0;
    }

    void switchState(CompileState newState);

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
class Construct : public spug::RCBase {

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

        typedef void (*CompileFunc)(crack::ext::Module *mod);
        typedef void (*InitFunc)();
    
    private:
        std::stack<builder::BuilderPtr> builderStack;

        // hook into the runtime module's uncaught exception function.        
        bool (*uncaughtExceptionFunc)();

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
         * Returns true if 'name' is a valid file.
         */
        static bool isFile(const std::string &name);

        /**
         * Returns true if 'name' is a valid directory.
         */
        static bool isDir(const std::string &name);
        
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
         * Creates a root context for the construct.
         */
        ContextPtr createRootContext();
        
    public:        

        // if true, emit warnings about things that have changed since the
        // last version of the language.
        bool migrationWarnings;
        
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
                   int32Type,
                   int64Type,
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
                   functionType;

        // Size of these PDNTs in bits.
        int intSize, intzSize;

        Construct(builder::Builder *rootBuilder, Construct *primary = 0);

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
         * Load the named module and returns it.  Returns null if the module 
         * could not be found, raises an exception if there were errors 
         * parsing the module.
         */
        ModuleDefPtr loadModule(StringVecIter moduleNameBegin,
                                StringVecIter moduleNameEnd,
                                std::string &canonicalName
                                );

        /**
         * a version of loadModule which accept a string canonicalName
         */
        ModuleDefPtr loadModule(const std::string &canonicalName);

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
         */
        int runScript(std::istream &src, const std::string &name);

        /**
         * Returns the current builder.
         */        
        builder::Builder &getCurBuilder();
};

} // namespace model

#endif

