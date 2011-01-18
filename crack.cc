// Copyright 2009 Google Inc

#include <iostream>
#include <fstream>
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "builder/llvm/LLVMBuilder.h"
#include "Crack.h"

using namespace std;

bool dump = false;

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage:" << endl;
        cerr << "  crack <script>" << endl;
        return 1;
    }

    Crack crack(new builder::mvll::LLVMBuilder());

    // default optimize
    crack.optimizeLevel = 2;

    // parse the main module
    char **arg = &argv[1];
    while (*arg) {
        if (!strcmp(*arg, "-")) {
            crack.setArgv(argc - (arg - argv), arg);
            crack.runScript(cin, "<stdin>");
            break;
        } else if (!strcmp(*arg, "-d")) {
            crack.dump = true;
        } else if (!strcmp(*arg, "-dg")) {
            crack.emitDebugInfo = true;
        } else if (!strcmp(*arg, "-O0")) {
            crack.optimizeLevel = 0;
        } else if (!strcmp(*arg, "-O1")) {
            crack.optimizeLevel = 1;
        } else if (!strcmp(*arg, "-O2")) {
            crack.optimizeLevel = 2;
        } else if (!strcmp(*arg, "-O3")) {
            crack.optimizeLevel = 3;
        } else if (!strcmp(*arg, "-n")) {
            crack.noBootstrap = true;
        } else if (!strcmp(*arg, "-g")) {
            crack.useGlobalLibs = false;
        } else if (!strcmp(*arg, "-m")) {
            crack.emitMigrationWarnings = true;
        } else if (!strcmp(*arg, "-l")) {
            ++arg;
            crack.addToSourceLibPath(*arg);
        } else {
            // it's the script name - run it.
            ifstream src(*arg);
            crack.setArgv(argc - (arg - argv), arg);
            crack.runScript(src, *arg);
            break;
        }
        ++arg;
    }
    
    crack.callModuleDestructors();
}
