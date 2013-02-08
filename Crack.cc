// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
    useGlobalLibs(true) {
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
    construct = new Construct(*this, builder);
}

void Crack::setCompileTimeBuilder(Builder *builder) {
    assert(construct && "no call to setBuilder");
    assert(builder->isExec() && "builder cannot be used compile time");    
    // we make a new options here, because compile time dump should be turned
    // so that compile time modules can run instead of dump
    // so that the _main_ builder can generate the correct ir to dump
    builder->options = new BuilderOptions(*options.get());
    builder->options->dumpMode = false;
    construct->compileTimeConstruct = new Construct(*this, builder, 
                                                    construct.get()
                                                    );
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
            ctc->loadBuiltinModules();
            if (!noBootstrap && !ctc->loadBootstrapModules())
                return false;
        }

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
    if (options->optionMap.find("out") == options->optionMap.end()) {
        if (name.substr(name.size() - 4) == ".crk")
            options->optionMap["out"] = name.substr(0, name.size() - 4);
        else
            // no extension - add one to the output file to distinguish it 
            // from the script.
            options->optionMap["out"] = name + ".bin";
    }
    return construct->runScript(src, name);
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

void Crack::printStats(std::ostream &out) {
    construct->stats->write(out);
    if (construct->compileTimeConstruct)
        construct->compileTimeConstruct->stats->write(out);
}
