// Copyright 2010 Google Inc.

#include "Crack.h"

#include <string.h>

#include "builder/Builder.h"
#include "model/Construct.h"
#include "model/Context.h"
#include "model/OverloadDef.h"

using namespace std;
using namespace model;
using namespace builder;

Crack::Crack(void) :
    initialized(false),    
    options(new builder::BuilderOptions()),
    noBootstrap(false),
    useGlobalLibs(true),
    emitMigrationWarnings(false) {

}

void Crack::addToSourceLibPath(const string &path) {
    assert(construct && "no call to setBuilder");
    construct->addToSourceLibPath(path);
    if (construct->compileTimeConstruct)
        construct->compileTimeConstruct->addToSourceLibPath(path);
}

void Crack::setArgv(int argc, char **argv) {
    assert(construct && "no call to setBuilder");
    construct->rootBuilder->setArgv(argc, argv);
}

void Crack::setBuilder(Builder *builder) {
    builder->options = options;
    construct = new Construct(builder);
}

void Crack::setCompileTimeBuilder(Builder *builder) {
    assert(construct && "no call to setBuilder");
    assert(builder->isExec() && "builder cannot be used compile time");
    builder->options = options;
    construct->compileTimeConstruct = new Construct(builder, construct.get());
}

bool Crack::init() {
    if (!initialized) {
        assert(construct && "no call to setBuilder");
        Construct *ctc = construct->compileTimeConstruct.get();

        // finalize the search path
        if (useGlobalLibs) {
            construct->addToSourceLibPath(".");
            construct->addToSourceLibPath(CRACKLIB);
            
            // XXX please refactor me
            if (ctc) {
                ctc->addToSourceLibPath(".");
                ctc->addToSourceLibPath(CRACKLIB);
            }
        }

        // initialize the compile-time construct first
        if (ctc) {
            ctc->migrationWarnings = emitMigrationWarnings;
            ctc->loadBuiltinModules();
            if (!noBootstrap && !ctc->loadBootstrapModules())
                return false;
        }

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
    options->optionMap["mainUnit"] = name;
    options->optionMap["outFile"] = basename(name.c_str());
    construct->runScript(src, name);
}

void Crack::callModuleDestructors() {

    // run through all of the destructors backwards.
    for (vector<ModuleDefPtr>::reverse_iterator ri = 
            construct->loadedModules.rbegin();
         ri != construct->loadedModules.rend();
         ++ri
         )
        (*ri)->callDestructor();
}
