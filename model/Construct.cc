// Copyright 2010 Google Inc.

#include "Construct.h"

#include <sys/stat.h>
#include <fstream>
#include <dlfcn.h>
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

using namespace std;
using namespace model;
using namespace parser;
using namespace builder;
using namespace crack::ext;

Construct::ModulePath Construct::searchPath(
    const Construct::StringVec &path,
    Construct::StringVecIter moduleNameBegin,
    Construct::StringVecIter moduleNameEnd,
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
    rootBuilder(builder) {

    createRootContext();
    
    // steal any stuff from the primary we want to use as a default.
    if (primary)
        sourceLibPath = primary->sourceLibPath;
}

void Construct::addToSourceLibPath(const string &path) {
    size_t pos = 0;
    size_t i = path.find(':');
    while (i != -1) {
        sourceLibPath.push_back(path.substr(pos, i - pos));
        pos = i + 1;
        i = path.find('.', pos);
    }
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
            initExtensionModule("crack.compiler", &compiler::init);
        rootContext->construct->crackContext = ccMod->lookUp("CrackContext");
        moduleCache["crack.compiler"] = ccMod;
        ccMod->finished = true;
        ns = ccMod;
    }

    // in either case, aliases get installed in the compiler namespace.
    rootContext->compileNS->addAlias(ns->lookUp("static").get());
    rootContext->compileNS->addAlias(ns->lookUp("final").get());
    rootContext->compileNS->addAlias(ns->lookUp("FILE").get());
    rootContext->compileNS->addAlias(ns->lookUp("LINE").get());
}

void Construct::parseModule(Context &context,
                            ModuleDef *module,
                            const std::string &path,
                            istream &src
                            ) {
    Toker toker(src, path.c_str());
    Parser parser(toker, &context);
    parser.parse();
    module->close(context);
}

ModuleDefPtr Construct::initExtensionModule(const string &canonicalName,
                                            Construct::InitFunc initFunc
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
    ModuleDefPtr modDef = context->createModule(canonicalName);
    Module mod(context.get());
    initFunc(&mod);
    mod.finish();
    modDef->close(*context);
    
    return modDef;
}

ModuleDefPtr Construct::loadSharedLib(const string &path, 
                                      Construct::StringVecIter moduleNameBegin,
                                      Construct::StringVecIter moduleNameEnd,
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
        modDef = context->createModule(canonicalName);
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
    }
        
    
    return true;
}

int Construct::runScript(istream &src, const string &name) {
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
    ModuleDefPtr modDef = context->createModule(name);
    try {
        parseModule(*context, modDef.get(), name, src);
        loadedModules.push_back(modDef);
    } catch (const ParseError &ex) {
        cerr << ex << endl;
        return 1;
    }
}
