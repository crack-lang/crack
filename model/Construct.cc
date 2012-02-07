// Copyright 2010 Google Inc.

#include "Construct.h"

#include <sys/stat.h>
#include <fstream>
#include <dlfcn.h>
#include <stdlib.h>
#include "parser/Parser.h"
#include "parser/ParseError.h"
#include "parser/Toker.h"
#include "builder/Builder.h"
#include "ext/Module.h"
#include "Context.h"
#include "GlobalNamespace.h"
#include "ModuleDef.h"
#include "StrConst.h"
#include "TypeDef.h"
#include "compiler/init.h"
#include "builder/util/CacheFiles.h"

using namespace std;
using namespace model;
using namespace parser;
using namespace builder;
using namespace crack::ext;

void ConstructStats::write(std::ostream &out) const {

    out << "\n----------------\n";
    out << "parsed     : " << parsedCount << "\n";
    out << "cached     : " << cachedCount << "\n";
    out << "startup    : " << timing[start] << "s\n";
    out << "builtin    : " << timing[builtin] << "s\n";
    out << "parse/build: " << timing[build] << "s\n";
    out << "run        : " << timing[run] << "s\n";
    double total = 0;
    for (ModuleTiming::const_iterator i = moduleTimes.begin();
         i != moduleTimes.end();
         ++i) {
         cout << i->first << ": " << i->second << "s\n";
         total += i->second;
    }
    out << "total module time: " << total << "s\n";
    out << endl;

}

void ConstructStats::switchState(CompileState newState) {
    struct timeval t;
    gettimeofday(&t, NULL);
    double beforeF = (lastTime.tv_usec/1000000.0) + lastTime.tv_sec;
    double nowF = (t.tv_usec/1000000.0) + t.tv_sec;
    double diff = (nowF - beforeF);
    timing[state] += diff;
    if (state == build && currentModule != "NONE") {
        // if we are saving build time, add it to the current module
        // as well
        moduleTimes[currentModule] += diff;
    }
    lastTime = t;
    state = newState;
}

Construct::ModulePath Construct::searchPath(
    const Construct::StringVec &path,
    Construct::StringVecIter moduleNameBegin,
    Construct::StringVecIter moduleNameEnd,
    const std::string &extension,
    int verbosity
) {
    // try to find a matching file.
    for (StringVecIter pathIter = path.begin();
         pathIter != path.end();
         ++pathIter
         ) {
        string fullName = joinName(*pathIter, moduleNameBegin, moduleNameEnd,
                                   extension
                                   );
        if (verbosity > 1) {
            cerr << "search: " << fullName << endl;
        }
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

bool Construct::isFile(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISREG(st.st_mode);
}

bool Construct::isDir(const std::string &name) {
    struct stat st;
    if (stat(name.c_str(), &st))
        return false;
    
    // XXX should check symlinks, too
    return S_ISDIR(st.st_mode);
}

std::string Construct::joinName(const std::string &base,
                                Construct::StringVecIter pathBegin,
                                Construct::StringVecIter pathEnd,
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

Construct::Construct(Builder *builder, Construct *primary) :
    rootBuilder(builder),
    uncaughtExceptionFunc(0) {

    if (builder->options->statsMode)
        stats = new ConstructStats();

    builderStack.push(builder);
    createRootContext();
    
    // steal any stuff from the primary we want to use as a default.
    if (primary)
        sourceLibPath = primary->sourceLibPath;
}

void Construct::addToSourceLibPath(const string &path) {
    size_t pos = 0;
    size_t i = path.find(':');
    while (i != -1) {
        if (i > 1 && path[i-1] == '/')
            sourceLibPath.push_back(path.substr(pos, i - pos - 1));
        else
            sourceLibPath.push_back(path.substr(pos, i - pos));
        pos = i + 1;
        i = path.find(':', pos);
    }
    if (path.size() > 1 && path[path.size()-1] == '/')
        sourceLibPath.push_back(path.substr(pos, (path.size()-pos)-1));
    else
        sourceLibPath.push_back(path.substr(pos));
}

ContextPtr Construct::createRootContext() {

    rootContext = new Context(*rootBuilder, Context::module, this,
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
    
    return rootContext;
}

void Construct::loadBuiltinModules() {
    // loads the compiler extension.  If we have a compile-time construct, 
    // the extension belongs to him and we just want to steal his defines.
    // Otherwise, we initialize them ourselves.
    NamespacePtr ns;
    if (compileTimeConstruct) {
        ns = compileTimeConstruct->rootContext->compileNS;
    } else {
        // initialize the built-in compiler extension and store the 
        // CrackContext type in global data.
        ModuleDefPtr ccMod = 
            initExtensionModule("crack.compiler", &compiler::init, 0);
        rootContext->construct->crackContext = ccMod->lookUp("CrackContext");
        moduleCache["crack.compiler"] = ccMod;
        ccMod->finished = true;
        ns = ccMod;
    }

    // in either case, aliases get installed in the compiler namespace.
    rootContext->compileNS->addAlias(ns->lookUp("static").get());
    rootContext->compileNS->addAlias(ns->lookUp("final").get());
    rootContext->compileNS->addAlias(ns->lookUp("abstract").get());
    rootContext->compileNS->addAlias(ns->lookUp("FILE").get());
    rootContext->compileNS->addAlias(ns->lookUp("LINE").get());
    rootContext->compileNS->addAlias(ns->lookUp("encoding").get());
    rootContext->compileNS->addAlias(ns->lookUp("export_symbols").get());

    // load the runtime extension
    StringVec crackRuntimeName(2);
    crackRuntimeName[0] = "crack";
    crackRuntimeName[1] = "runtime";
    string name;
    ModuleDefPtr rtMod = rootContext->construct->loadModule(
                                                     crackRuntimeName.begin(),
                                                     crackRuntimeName.end(),
                                                     name
                                                     );
    if (!rtMod) {
        cerr << "failed to load crack runtime from module load path" << endl;
        // XXX exception?
        exit(1);
    }
    // alias some basic builtins from runtime
    // mostly for legacy reasons
    VarDefPtr a = rtMod->lookUp("puts");
    assert(a && "no puts in runtime");
    rootContext->ns->addAlias("puts", a.get());
    a = rtMod->lookUp("__die");
    assert(a && "no __die in runtime");
    rootContext->ns->addAlias("__die", a.get());
    rootContext->compileNS->addAlias("__die", a.get());
    a = rtMod->lookUp("printint");
    if (a)
        rootContext->ns->addAlias("printint", a.get());
    
    // for jit builders, get the uncaught exception handler
    if (rootBuilder->isExec()) {
        FuncDefPtr uncaughtExceptionFuncDef =
            rootContext->lookUpNoArgs("__CrackUncaughtException", true,
                                    rtMod.get()
                                    );
        if (uncaughtExceptionFuncDef)
            uncaughtExceptionFunc = 
                reinterpret_cast<bool (*)()>(
                    uncaughtExceptionFuncDef->getFuncAddr(*rootBuilder)
                );
        else
            cerr << "Uncaught exception function not found in runtime!" << 
                endl;
    }
}

void Construct::parseModule(Context &context,
                            ModuleDef *module,
                            const std::string &path,
                            istream &src
                            ) {
    Toker toker(src, path.c_str());
    Parser parser(toker, &context);
    string lastModule;
    ConstructStats::CompileState oldStatState;
    if (rootBuilder->options->statsMode) {
        stats->switchState(ConstructStats::build);
        lastModule = stats->currentModule;
        stats->currentModule = module->getFullName();
        oldStatState = context.construct->stats->state;
        stats->parsedCount++;
    }
    parser.parse();
    if (rootBuilder->options->statsMode) {
        stats->switchState(oldStatState);
        stats->currentModule = lastModule;
    }
    module->close(context);
}

ModuleDefPtr Construct::initExtensionModule(const string &canonicalName,
                                            Construct::CompileFunc compileFunc,
                                            Construct::InitFunc initFunc
                                            ) {
    // create a new context
    BuilderPtr builder = rootBuilder->createChildBuilder();
    builderStack.push(builder);
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get(),
                    new GlobalNamespace(rootContext->ns.get(), canonicalName),
                    0 // we don't need a compile namespace
                    );
    context->toplevel = true;

    // create a module object
    ModuleDefPtr modDef = context->createModule(canonicalName);
    Module mod(context.get());
    compileFunc(&mod);
    modDef->fromExtension = true;
    mod.finish();
    modDef->close(*context);
    builderStack.pop();

    if (initFunc)
        initFunc();

    return modDef;
}

namespace {
    // load a function from a shared library
    void *loadFunc(void *handle, const string &path, const string &funcName) {
        void *func = dlsym(handle, funcName.c_str());
        
        if (!func) {
            cerr << "Error looking up function " << funcName
                << " in extension library " << path << ": "
                << dlerror() << endl;
            return 0;
        } else {
            return func;
        }
    }
}

ModuleDefPtr Construct::loadSharedLib(const string &path, 
                                      Construct::StringVecIter moduleNameBegin,
                                      Construct::StringVecIter moduleNameEnd,
                                      string &canonicalName
                                      ) {

    void *handle = rootBuilder->loadSharedLibrary(path);

    // construct the full init function name
    // XXX should do real name mangling. also see LLVMLinkerBuilder::initializeImport
    std::string initFuncName;
    for (StringVecIter iter = moduleNameBegin;
         iter != moduleNameEnd;
         ++iter
         )
        initFuncName += *iter + '_';

    CompileFunc cfunc = (CompileFunc)loadFunc(handle, path, 
                                              initFuncName + "cinit"
                                              );
    InitFunc rfunc = (InitFunc)loadFunc(handle, path, initFuncName + "rinit");
    if (!cfunc || !rfunc)
        return 0;

    return initExtensionModule(canonicalName, cfunc, rfunc);
}

ModuleDefPtr Construct::loadModule(const string &canonicalName) {

    StringVec name;

    size_t pos = 0;
    size_t i = canonicalName.find('.');
    while (i != -1) {
        name.push_back(canonicalName.substr(pos, i - pos));
        pos = i + 1;
        i = canonicalName.find('.', pos);
    }
    name.push_back(canonicalName.substr(pos));

    string cname;
    ModuleDefPtr m = loadModule(name.begin(), name.end(), cname);
    assert(cname == canonicalName && "loadModule canonicalName mismatch");
    return m;

}

ModuleDefPtr Construct::loadFromCache(const string &canonicalName,
                                      const string &path
                                      ) {
    if (!rootBuilder->options->cacheMode)
        return 0;

    // create a new builder, context and module
    BuilderPtr builder = rootBuilder->createChildBuilder();
    builderStack.push(builder);
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

    ModuleDefPtr modDef = context->materializeModule(canonicalName, path);
    if (modDef && rootBuilder->options->statsMode)
        stats->cachedCount++;
    
    builderStack.pop();
    return modDef;
}

ModuleDefPtr Construct::loadModule(Construct::StringVecIter moduleNameBegin,
                                   Construct::StringVecIter moduleNameEnd,
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
    Construct::ModuleMap::iterator iter = moduleCache.find(canonicalName);
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
                                    ".so",
                                    rootBuilder->options->verbosity
                                    );
    
    ModuleDefPtr modDef;
    if (modPath.found && !modPath.isDir) {
        modDef = loadSharedLib(modPath.path, moduleNameBegin,
                               moduleNameEnd,
                               canonicalName
                               );
        moduleCache[canonicalName] = modDef;
    } else {
        
        // fall through - not in immediate module cache, not a shared library

        // try to find the module on the source path
        modPath = searchPath(sourceLibPath, moduleNameBegin, moduleNameEnd, 
                            ".crk", rootBuilder->options->verbosity
                            );
        if (!modPath.found)
            return 0;
    
        // create a new builder, context and module
        BuilderPtr builder = rootBuilder->createChildBuilder();
        builderStack.push(builder);
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

        // before parsing the module from scratch, we give the builder a chance
        // to materialize the module through its own means (e.g., a cache)
        bool cached = false;
        if (rootBuilder->options->cacheMode && !modPath.isDir)
            modDef = context->materializeModule(canonicalName, modPath.path);
        if (modDef) {
            cached = true;
            if (rootBuilder->options->statsMode)
                stats->cachedCount++;
        } else {
            modDef = context->createModule(canonicalName, modPath.path);
        }

        moduleCache[canonicalName] = modDef;

        if (!cached) {
            if (!modPath.isDir) {
                ifstream src(modPath.path.c_str());
                // parse from scratch
                parseModule(*context, modDef.get(), modPath.path, src);
            } else {
                // directory
                modDef->close(*context);
            }
        } else {
            // XXX hook to run/finish cached module
        }

        builderStack.pop();
    }

    modDef->finished = true;
    loadedModules.push_back(modDef);
    return modDef;
}    

void Construct::registerModule(ModuleDef *module) {
    moduleCache[module->getFullName()] = module;
    loadedModules.push_back(module);
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

bool Construct::loadBootstrapModules() {
    try {
        StringVec crackLangName(2);
        crackLangName[0] = "crack";
        crackLangName[1] = "lang";
        string name;
        ModuleDefPtr mod = loadModule(crackLangName.begin(),
                                      crackLangName.end(), 
                                      name
                                      );
        
        if (!mod) {
            cerr << "Bootstrapping module crack.lang not found." << endl;
            return false;
        }

        // extract the basic types from the module context
        rootContext->construct->objectType = extractClass(mod.get(), "Object");
        rootContext->ns->addAlias(rootContext->construct->objectType.get());
        rootContext->construct->stringType = extractClass(mod.get(), "String");
        rootContext->ns->addAlias(rootContext->construct->stringType.get());
        rootContext->construct->staticStringType = 
            extractClass(mod.get(), "StaticString");
        rootContext->ns->addAlias(
            rootContext->construct->staticStringType.get()
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
        
        return rootContext->construct->objectType && 
               rootContext->construct->stringType;
    } catch (const ParseError &ex) {
        cerr << ex << endl;
        return false;
    } catch (...) {
        if (!uncaughtExceptionFunc)
            cerr << "Uncaught exception, no uncaught exception handler!" << 
                endl;
        else if (!uncaughtExceptionFunc())
            cerr << "Unknown exception caught." << endl;
    }
        
    
    return true;
}

int Construct::runScript(istream &src, const string &name) {
    // create the builder and context for the script.
    BuilderPtr builder = rootBuilder->createChildBuilder();
    builderStack.push(builder);
    ContextPtr context =
        new Context(*builder, Context::module, rootContext.get(),
                    new GlobalNamespace(rootContext->ns.get(), name)
                    );
    context->toplevel = true;

    // XXX using the name as the canonical name which is not right, need to 
    // produce a canonical name from the file name, e.g. "foo" -> "foo", 
    // "foo.crk" -> foo, "anything weird" -> "__main__" or something.

    ModuleDefPtr modDef;
    bool cached = false;
    if (rootBuilder->options->cacheMode)
        builder::initCacheDirectory(rootBuilder->options.get());
    // we check cacheMode again after init,
    // because it might have been disabled if
    // we couldn't find an appropriate cache directory
    if (rootBuilder->options->cacheMode) {
        modDef = context->materializeModule(name, name);
    }
    if (modDef) {
        cached = true;
        loadedModules.push_back(modDef);
        if (rootBuilder->options->statsMode)
            stats->cachedCount++;
    }
    else
        modDef = context->createModule(name, name);

    try {
        if (!cached) {
            parseModule(*context, modDef.get(), name, src);
            loadedModules.push_back(modDef);
        } else {
            // XXX hook to run/finish cached module
        }
    } catch (const ParseError &ex) {
        cerr << ex << endl;
        return 1;
    } catch (...) {
        if (!uncaughtExceptionFunc)
            cerr << "Uncaught exception, no uncaught exception handler!" <<
                endl;
        else if (!uncaughtExceptionFunc())
            cerr << "Unknown exception caught." << endl;
    }

    builderStack.pop();
    rootBuilder->finishBuild(*context);
    if (rootBuilder->options->statsMode)
        stats->switchState(ConstructStats::end);
    return 0;
}

builder::Builder &Construct::getCurBuilder() {
    return *builderStack.top();
}

void Construct::registerDef(VarDef *def) {
    registry[def->getFullName()] = def;
}

VarDefPtr Construct::getRegisteredDef(const std::string &name) {
    VarDefMap::iterator iter = registry.find(name);
    if (iter != registry.end())
        return iter->second;
    else
        return 0;
}