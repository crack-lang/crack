// Copyright 2010 Google Inc.

#include "Crack.h"

#include "builder/Builder.h"
#include "model/Construct.h"
#include "model/Context.h"

using namespace std;
using namespace model;
using namespace builder;

Crack::Crack(Builder *builder) : 
    initialized(false),
    dump(false),
    optimizeLevel(0),
    emitDebugInfo(false),
    noBootstrap(false),
    useGlobalLibs(true),
    emitMigrationWarnings(false) {

    construct = new Construct(builder);
}

void Crack::addToSourceLibPath(const string &path) {
    construct->addToSourceLibPath(path);
}

void Crack::setArgv(int argc, char **argv) {
    construct->rootBuilder->setArgv(argc, argv);
}

bool Crack::init() {
    if (!initialized) {
        // finalize the search path
        if (useGlobalLibs) {
            construct->addToSourceLibPath(".");
            construct->addToSourceLibPath(CRACKLIB);
        }
        
        construct->rootBuilder->setOptimize(optimizeLevel);

        // pass the emitMigrationWarnings flag down to the global data.
        construct->migrationWarnings = emitMigrationWarnings;

        construct->loadBuiltinModules();
        if (!noBootstrap && !construct->loadBootstrapModules())
            return false;
        initialized = true;
    }
    
    return true;
}

int Crack::runScript(std::istream &src, const std::string &name) {
    if (!init())
        return 1;
    construct->runScript(src, name);
}

void Crack::callModuleDestructors() {
    // if we're in dump mode, nothing got run and nothing needs cleanup.
    if (dump) return;

    // run through all of the destructors backwards.
    for (vector<ModuleDefPtr>::reverse_iterator ri = 
            construct->loadedModules.rbegin();
         ri != construct->loadedModules.rend();
         ++ri
         )
        (*ri)->callDestructor();
}
