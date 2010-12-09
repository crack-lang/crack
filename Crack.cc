// Copyright 2010 Google Inc.

#include "Crack.h"

#include <sys/stat.h>
#include <fstream>
#include <dlfcn.h>
#include "model/Context.h"
#include "model/GlobalNamespace.h"
#include "model/ModuleDef.h"
#include "model/TypeDef.h"
#include "parser/Parser.h"
#include "parser/ParseError.h"
#include "parser/Toker.h"
#include "builder/Builder.h"
#include "builder/llvm/LLVMBuilder.h"
#include "ext/Module.h"
#include "compiler/init.h"

using namespace std;
using namespace model;
using namespace parser;
using namespace builder;
using namespace builder::mvll;
using namespace crack::ext;

Crack *Crack::theInstance = 0;

Crack &Crack::getInstance() {
    if (!theInstance)
        theInstance = new Crack();
    
    return *theInstance;
}

void Crack::addToSourceLibPath(const string &path) {
    size_t pos = 0;
    size_t i = path.find(':');
    while (i != -1) {
        sourceLibPath.push_back(path.substr(pos, i - pos));
        pos = i + 1;
        i = path.find('.', pos);
    }
    sourceLibPath.push_back(path.substr(pos));
}

Crack::Crack() : 
    sourceLibPath(1), 
    rootBuilder(new LLVMBuilder()), 
    initialized(false),
    dump(false),
    optimizeLevel(0),
    emitDebugInfo(false),
    noBootstrap(false),
    useGlobalLibs(true),
    emitMigrationWarnings(false) {

    rootContext = new Context(*rootBuilder, Context::module, (Context *)0,
                              new GlobalNamespace(0, ""),
                              new GlobalNamespace(0, "")
                              );

    // register the primitives into our builtin module
    ContextPtr builtinContext =
        new Context(*rootBuilder, Context::module, rootContext.get(),
                    // NOTE we can't have rootContext namespace be the parent
                    // here since we are adding the aliases into rootContext
                    // and we get dependency issues
                    new GlobalNamespace(0, ".builtin"),
                    new GlobalNamespace(0, ".builtin"));
    builtinMod = rootBuilder->registerPrimFuncs(*builtinContext);

    // alias builtins to the root namespace
    for (Namespace::VarDefMap::iterator i = builtinContext->ns->beginDefs();
         i != builtinContext->ns->endDefs();
         ++i) {
         rootContext->ns->addAlias(i->first, i->second.get());
    }

    // search for source files in the current directory
    sourceLibPath[0] = ".";
}

bool Crack::init() {
    if (!initialized) {
        // finalize the search path
        if (useGlobalLibs)
            sourceLibPath.push_back(CRACKLIB);
        
        // initialize the built-in compiler extension and store the 
        // CrackContext type in global data.
        ModuleDefPtr ccMod = 
            initExtensionModule("crack.compiler", &compiler::init);
        rootContext->globalData->crackContext = ccMod->lookUp("CrackContext");
        moduleCache["crack.compiler"] = ccMod;
        ccMod->finished = true;
        rootContext->compileNS->addAlias(ccMod->lookUp("static").get());
        rootContext->compileNS->addAlias(ccMod->lookUp("final").get());
        rootContext->compileNS->addAlias(ccMod->lookUp("FILE").get());
        rootContext->compileNS->addAlias(ccMod->lookUp("LINE").get());

        // load the bootstrapping modules - library files that are essential 
        // to the language, like the definitions of the Object and String 
        // classes.
        if (!noBootstrap && !loadBootstrapModules())
            return false;

        // pass the emitMigrationWarnings flag down to the global data.
        rootContext->globalData->migrationWarnings = emitMigrationWarnings;

        initialized = true;
    }
    
    return true;
}

//Crack::~Crack() {}

std::string Crack::joinName(const std::string &base,
                            Crack::StringVecIter pathBegin,
                            Crack::StringVecIter pathEnd,
                            const std::string &ext
                            ) {
    // not worrying about performance, this only happens during module load.
    string result = base;
    
    // base off current directory if an empty path was specified.
    if (!result.size())
        result = ".";

    for (StringVecIter iter = pathBegin;
         iter != pathEnd;
         ++iter
         ) {
        result += "/" + *iter;
    }

    return result + ext;
}

bool Crack::isFile(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISREG(st.st_mode);
}

bool Crack::isDir(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISDIR(st.st_mode);
}

Crack::ModulePath Crack::searchPath(const Crack::StringVec &path,
                                    Crack::StringVecIter moduleNameBegin,
                                    Crack::StringVecIter moduleNameEnd,
                                    const std::string &extension
                                    ) {
    // try to find a matching file.
    for (StringVecIter pathIter = path.begin();
         pathIter != path.end();
         ++pathIter
         ) {
        string fullName = joinName(*pathIter, moduleNameBegin, moduleNameEnd,
                                   extension
                                   );
        if (isFile(fullName))
            return ModulePath(fullName, true, false);
    }
    
    // try to find a matching directory.
    string empty;
    for (StringVecIter pathIter = path.begin();
         pathIter != path.end();
         ++pathIter
         ) {
        string fullName = joinName(*pathIter, moduleNameBegin, moduleNameEnd,
                                   empty
                                   );
        if (isDir(fullName))
            return ModulePath(fullName, true, true);
    }
    
    return ModulePath(empty, false, false);
}

void Crack::parseModule(Context &context,
                        ModuleDef *module,
                        const std::string &path,
                        istream &src
                        ) {
    Toker toker(src, path.c_str());
    Parser parser(toker, &context);
    parser.parse();
    context.builder.setOptimize(optimizeLevel);
    module->close(context);
    if (dump)
        context.builder.dump();
    else
        context.builder.run();
}

ModuleDefPtr Crack::initExtensionModule(const string &canonicalName,
                                        Crack::InitFunc initFunc
                                        ) {
    // create a new context
    BuilderPtr builder = rootBuilder->createChildBuilder();
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get(),
                    new GlobalNamespace(rootContext->ns.get(), canonicalName),
                    0 // we don't need a compile namespace
                    );
    context->toplevel = true;

    // create a module object
    ModuleDefPtr modDef = context->createModule(canonicalName, emitDebugInfo);
    Module mod(context.get());
    initFunc(&mod);
    mod.finish();
    modDef->close(*context);
    
    if (dump)
        builder->dump();
    
    return modDef;
}

ModuleDefPtr Crack::loadSharedLib(const string &path, 
                                  Crack::StringVecIter moduleNameBegin,
                                  Crack::StringVecIter moduleNameEnd,
                                  string &canonicalName
                                  ) {
    void *handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        cerr << "opening library " << path << ": " << dlerror() << endl;
        return 0;
    }
    
    // construct the full init function name XXX should do real name mangling
    std::string initFuncName;
    for (StringVecIter iter = moduleNameBegin;
         iter != moduleNameEnd;
         ++iter
         )
        initFuncName += *iter + '_';
    
    initFuncName += "init";

    InitFunc func = (InitFunc)dlsym(handle, initFuncName.c_str());
    
    if (!func) {
        cerr << "Error looking up function " << initFuncName
            << " in extension library " << path << ": "
            << dlerror() << endl;
        return 0;
    }
    
    return initExtensionModule(canonicalName, func);
}

ModuleDefPtr Crack::loadModule(Crack::StringVecIter moduleNameBegin,
                               Crack::StringVecIter moduleNameEnd,
                               string &canonicalName
                               ) {
    // create the dotted canonical name of the module
    for (StringVecIter iter = moduleNameBegin;
         iter != moduleNameEnd;
         ++iter
         )
        if (canonicalName.size())
            canonicalName += "." + *iter;
        else
            canonicalName = *iter;

    // check to see if we have it in the cache
    ModuleMap::iterator iter = moduleCache.find(canonicalName);
    if (iter != moduleCache.end())
        return iter->second;

    // load the parent module
    StringVec::const_reverse_iterator rend(moduleNameEnd);
    ++rend;
    if (rend != StringVec::const_reverse_iterator(moduleNameBegin)) {
        StringVecIter last(rend.base());
        string parentCanonicalName;
        ModuleDefPtr parent = loadModule(moduleNameBegin, last, 
                                         parentCanonicalName
                                         );
    }
    
    // look for a shared library
    ModulePath modPath = searchPath(sourceLibPath, moduleNameBegin,
                                    moduleNameEnd,
                                    ".so"
                                    );
    
    ModuleDefPtr modDef;
    if (modPath.found && !modPath.isDir) {
        modDef = loadSharedLib(modPath.path, moduleNameBegin,
                               moduleNameEnd,
                               canonicalName
                               );
        moduleCache[canonicalName] = modDef;
    } else {
        
        // try to find the module on the source path
        modPath = searchPath(sourceLibPath, moduleNameBegin, moduleNameEnd, 
                            ".crk"
                            );
        if (!modPath.found)
            return 0;
    
        // create a new builder, context and module
        BuilderPtr builder = rootBuilder->createChildBuilder();
        ContextPtr context =
            new Context(*builder, Context::module, rootContext.get(),
                        new GlobalNamespace(rootContext->ns.get(), 
                                            canonicalName
                                            ),
                        new GlobalNamespace(rootContext->compileNS.get(),
                                            canonicalName
                                            )
                        );
        context->toplevel = true;
        modDef = context->createModule(canonicalName, emitDebugInfo);
        moduleCache[canonicalName] = modDef;
        if (!modPath.isDir) {
            ifstream src(modPath.path.c_str());
            parseModule(*context, modDef.get(), modPath.path, src);
        }
    }

    modDef->finished = true;
    loadedModules.push_back(modDef);
    return modDef;
}    

namespace {
    // extract a class from a module and verify that it is a class - returns 
    // null on failure.
    TypeDef *extractClass(ModuleDef *mod, const char *name) {
        VarDefPtr var = mod->lookUp(name);
        TypeDef *type;
        if (var && (type = TypeDefPtr::rcast(var))) {
            return type;
        } else {
            cerr << name << " class not found in module crack.lang" << endl;
            return 0;
        }
    }
}

bool Crack::loadBootstrapModules() {
    try {
        StringVec crackLangName(2);
        crackLangName[0] = "crack";
        crackLangName[1] = "lang";
        string name;
        ModuleDefPtr mod = loadModule(crackLangName, name);
        
        if (!mod) {
            cerr << "Bootstrapping module crack.lang not found." << endl;
            return false;
        }

        // extract the basic types from the module context
        rootContext->globalData->objectType = extractClass(mod.get(), "Object");
        rootContext->ns->addAlias(rootContext->globalData->objectType.get());
        rootContext->globalData->stringType = extractClass(mod.get(), "String");
        rootContext->ns->addAlias(rootContext->globalData->stringType.get());
        rootContext->globalData->staticStringType = 
            extractClass(mod.get(), "StaticString");
        rootContext->ns->addAlias(
            rootContext->globalData->staticStringType.get()
        );

        // replace the bootstrapping context with a new context that 
        // delegates to the original root context - this is the "bootstrapped 
        // context."  It contains all of the special definitions that were 
        // extracted from the bootstrapping modules.
        rootContext = rootContext->createSubContext(Context::module);
        
        // extract some constants
        VarDefPtr v = mod->lookUp("true");
        if (v)
            rootContext->ns->addAlias(v.get());
        v = mod->lookUp("false");
        if (v)
            rootContext->ns->addAlias(v.get());
        v = mod->lookUp("print");
        if (v)
            rootContext->ns->addAlias(v.get());
        
        return rootContext->globalData->objectType && 
               rootContext->globalData->stringType;
    } catch (const ParseError &ex) {
        cerr << ex << endl;
        return false;
    }
        
    
    return true;
}

void Crack::setArgv(int argc, char **argv) {
    rootBuilder->setArgv(argc, argv);
}

int Crack::runScript(std::istream &src, const std::string &name) {
    // finalize all initialization
    if (!init())
        return 1;

    // create the builder and context for the script.
    BuilderPtr builder = rootBuilder->createChildBuilder();
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get(),
                    new GlobalNamespace(rootContext->ns.get(), name),
                    new GlobalNamespace(rootContext->compileNS.get(), name)
                    );
    context->toplevel = true;

    // XXX using the name as the canonical name which is not right, need to 
    // produce a canonical name from the file name, e.g. "foo" -> "foo", 
    // "foo.crk" -> foo, "anything weird" -> "__main__" or something.
    ModuleDefPtr modDef = context->createModule(name, emitDebugInfo);
    try {
        parseModule(*context, modDef.get(), name, src);
        loadedModules.push_back(modDef);
    } catch (const ParseError &ex) {
        cerr << ex << endl;
        return 1;
    }
}

ModuleDefPtr Crack::loadModule(const vector<string> &moduleName,
                               string &canonicalName
                               ) {
    return getInstance().loadModule(moduleName.begin(), 
                                    moduleName.end(), 
                                    canonicalName
                                    );
}

void Crack::callModuleDestructors() {
    // if we're in dump mode, nothing got run and nothing needs cleanup.
    if (dump) return;

    // run through all of the destructors backwards.
    for (vector<ModuleDefPtr>::reverse_iterator ri = loadedModules.rbegin();
         ri != loadedModules.rend();
         ++ri
         )
        (*ri)->callDestructor();
}
