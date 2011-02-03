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
#include "builder/llvm/LLVMLinkerBuilder.h"
#include "builder/llvm/LLVMJitBuilder.h"
#include "Crack.h"

using namespace std;

struct option longopts[] = {
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
        cerr << "  crackc <script>" << endl;
        return 1;
    }

    Crack crack(new builder::mvll::LLVMLinkerBuilder());
    crack.setCompileTimeBuilder(new builder::mvll::LLVMJitBuilder());

    // default optimize
    crack.optimizeLevel = 2;

    // parse the main module
    int opt;
    bool optionsError = false;
    while ((opt = getopt_long(argc, argv, "dgO:nGml:", longopts, NULL)) != -1) {
        switch (opt) {
            case 0:
                // long option tied to a flag variable
                break;
            case '?':
                optionsError = true;
                break;
            case 'd':
                crack.dump = true;
                break;
            case 'g':
                crack.emitDebugInfo = true;
                break;
            case 'O':
                if (!*optarg || *optarg > '3' || *optarg < '0' || optarg[1]) {
                    cerr << "Bad value for -O/--optimize: " << optarg
                        << "expected 0-3" << endl;
                    exit(1);
                }
                
                crack.optimizeLevel = atoi(optarg);
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
                crack.addToSourceLibPath(optarg);
                break;
        }
    }
    
    // check for options errors
    if (optionsError)
        exit(1);

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
    
    crack.callModuleDestructors();
}
