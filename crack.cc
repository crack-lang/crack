// Copyright 2009 Google Inc

#include <iostream>
#include <fstream>
#include <getopt.h>
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "builder/llvm/LLVMJitBuilder.h"
#include "builder/llvm/LLVMLinkerBuilder.h"
#include "Crack.h"

using namespace std;

typedef enum {
    jitBuilder,
    nativeBuilder
} builderType;

struct option longopts[] = {
    {"builder", true, 0, 'B'},
    {"builder-opts", true, 0, 'b'},
    {"dump", false, 0, 'd'},
    {"debug", false, 0, 'g'},
    {"optimize", true, 0, 'O'},
    {"no-bootstrap", false, 0, 'n'},
    {"no-default-paths", false, 0, 'G'},
    {"migration-warnings", false, 0, 'm'},
    {"lib", true, 0, 'l'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv) {

    if (argc < 2) {
        cerr << "Usage:" << endl;
        cerr << "  crack [options] <script>" << endl;
        return 1;
    }


    // top level interface
    Crack crack;
    crack.options->optimizeLevel = 2;

    builderType bType = jitBuilder;
    string libPath;

    // parse the main module
    int opt;
    bool optionsError = false;
    while ((opt = getopt_long(argc, argv, "B:b:dgO:nGml:", longopts, NULL)) != -1) {
        switch (opt) {
            case 0:
                // long option tied to a flag variable
                break;
            case '?':
                optionsError = true;
                break;
            case 'B':
                cerr << "use builder: " << optarg << "\n";
                break;
            case 'b':
                cerr << "use builder opts: " << optarg << "\n";
                break;
            case 'd':
                crack.options->dumpMode = true;
                break;
            case 'g':
                crack.options->debugMode = true;
                break;
            case 'O':
                if (!*optarg || *optarg > '3' || *optarg < '0' || optarg[1]) {
                    cerr << "Bad value for -O/--optimize: " << optarg
                        << "expected 0-3" << endl;
                    exit(1);
                }
                
                crack.options->optimizeLevel = atoi(optarg);
                break;
            case 'n':
                crack.noBootstrap = true;
                break;
            case 'G':
                crack.useGlobalLibs = false;
                break;
            case 'm':
                crack.emitMigrationWarnings = true;
                break;
            case 'l':
                if (libPath.empty()) {
                    libPath = optarg;
                }
                else {
                    libPath.push_back(':');
                    libPath.append(optarg);
                }
                break;
        }
    }
    
    // check for options errors
    if (optionsError)
        exit(1);

    if (bType == jitBuilder) {
        // immediate execution in JIT
        crack.setBuilder(new builder::mvll::LLVMJitBuilder());
    }
    else {
        // compile to native binary
        crack.setBuilder(new builder::mvll::LLVMLinkerBuilder());
        crack.setCompileTimeBuilder(new builder::mvll::LLVMJitBuilder());
    }

    if (!libPath.empty())
        crack.addToSourceLibPath(libPath);

    // are there any more arguments?
    if (optind == argc) {
        cerr << "You need to define a script or the '-' option to read "
                "from standard input." << endl;
    } else if (!strcmp(argv[optind], "-")) {
        crack.setArgv(argc - optind, &argv[optind]);
        crack.runScript(cin, "<stdin>");
    } else {
        // it's the script name - run it.
        ifstream src(argv[optind]);
        crack.setArgv(argc - optind, &argv[optind]);
        crack.runScript(src, argv[optind]);
    }
    
    if (bType == jitBuilder && !crack.options->dumpMode)
        crack.callModuleDestructors();

}
