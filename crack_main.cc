// Copyright 2011-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include <iostream>
#include <fstream>
#include <getopt.h>
#include <libgen.h>
#include "spug/Tracer.h"
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/Construct.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/Serializer.h"
#include "model/TypeDef.h"
#include "builder/BuilderOptions.h"
#include "builder/mdl/ModelBuilder.h"
#include "builder/llvm/LLVMJitBuilder.h"
#include "builder/llvm/LLVMLinkerBuilder.h"
#include "builder/llvm/StructResolver.h"
#include "debug/DebugTools.h"
#include "Crack.h"
#include "config.h"

#ifdef __APPLE__
extern char *suboptarg; // getsubopt
#endif

using namespace std;
using spug::Tracer;

typedef enum {
    jitBuilder,
    nativeBuilder,
    modelBuilder,
    doubleBuilder = 1001,
    dumpFuncTable = 1002,
} builderType;

struct option longopts[] = {
    {"builder", true, 0, 'B'},
    {"builder-opts", true, 0, 'b'},
    {"dump", false, 0, 'd'},
    {"help", false, 0, 'h'},
    {"debug", false, 0, 'g'},
    {"double-builder", false, 0, doubleBuilder},
    {"optimize", true, 0, 'O'},
    {"verbosity", false, 0, 'v'},
    {"quiet", false, 0, 'q'},
    {"no-cache", false, 0, 'K'},
    {"no-bootstrap", false, 0, 'n'},
    {"no-default-paths", false, 0, 'G'},
    {"migration-warnings", false, 0, 'm'},
    {"lib", true, 0, 'l'},
    {"version", false, 0, 0},
    {"stats", false, 0, 0},
    {"dump-func-table", false, 0, dumpFuncTable},
    {"trace", true, 0, 't'},
    {0, 0, 0, 0}
};

static std::string prog;

void version() {
    cout << prog << " " << VERSION << endl;
}

void usage(int retval) {
    version();
    cout << "Usage:" << endl;
    cout << "  " << prog << " [options] <source file>" << endl;
    cout << " -B <name>  --builder\n    Main builder to use (llvm-jit or"
            " llvm-native)" << endl;
    cout << " -b <opts>  --builder-opts\n    Builder options in the form "
            "foo=bar,baz=bip" << endl;
    cout << " -d --dump\n    Dump IR to stdout instead of running or "
            "compiling" << endl;
    cout << " --double-builder\n    Run multiple internal builders, even in "
            "JIT mode." << endl;
    cout << " -G --no-default-paths\n    Do not include default module"
            " search paths" << endl;

    cout << " -C\n    Turn on module caching (already enabled by" << endl;
    cout << "    default, this just overrrides the CRACK_CACHING" << endl;
    cout << "    environment variable if it's set to false." << endl;
    cout << " -K\n    Disable module caching." << endl;

    cout << " -g --debug\n    Generate DWARF debug information" << endl;
    cout << " -O <N> --optimize\n    Use optimization level N (default 2)" << 
        endl;
    cout << " -l <path> --lib\n    Add directory to module search path" << endl;
    cout << " -m --migration-warnings\n    Include migration warnings" << endl;
    cout << " -n --no-bootstrap\n    Do not load bootstrapping modules" << 
        endl;
    cout << " -v --verbose\n    Verbose output, use more than once for "
            "greater effect" << endl;
    cout << " -q --quiet\n    No extra output, implies verbose level 0" << endl;
    cout << " --version\n    Emit the version number and exit" << endl;
    cout << " --stats\n    Emit statistics about compile time operations." << 
        endl;
    cout << " --dump-func-table\n    Dump the debug function table." << endl;
    cout << " -t <module> --trace <module>\n    Turn tracing on for the "
            "module." << endl;
    cout << "    Modules supporting tracing:" << endl;
    map<string, string> traceModules = Tracer::getTracers();
    for (map<string, string>::iterator i = traceModules.begin();
         i != traceModules.end();
         ++i
         ) {
        cout << "        " << i->first << endl;
        cout << "          " << i->second << endl;
    }
    exit(retval);
}

int main(int argc, char **argv) {

    int rc = 0;
    prog = basename(argv[0]);

    if (argc < 2)
        usage(0);

    // top level interface
    Crack crack;
    crack.options->optimizeLevel = 2;

    builderType bType = (prog == "crackc") ?
                                nativeBuilder:
                                jitBuilder;

    string libPath;
    if (getenv("CRACK_LIB_PATH"))
        libPath = getenv("CRACK_LIB_PATH");
    
    if (const char *caching = getenv("CRACK_CACHING")) {
        if (!strcasecmp(caching, "true")) {
            crack.cacheMode = true;
        } else if (!strcasecmp(caching, "false")) {
            crack.cacheMode = false;
        } else {
            cerr << "Illegal value for 'CRACK_CACHING', must be 'true' or "
                    "'false'." << endl;
            exit(1);
        }
    }

    // parse the main module
    int opt, idx;
    char *subopts, *value;
    char * const token[] = { NULL };
    bool optionsError = false;
    bool useDoubleBuilder = false;    
    bool doDumpFuncTable = false;
    while ((opt = getopt_long(argc, argv, "+B:b:dgO:nCKGml:vqt:", longopts, 
                              &idx
                              )
            ) != -1
           ) {
        switch (opt) {
            case 0:
                // long option tied to a flag variable
                if (strcmp(longopts[idx].name,"version") == 0) {
                    version();
                    exit(0);
                }
                if (strcmp(longopts[idx].name,"stats") == 0) {
                    crack.options->statsMode = true;
                }
                break;
            case '?':
                optionsError = true;
                break;
            case 'B':
                if (strncmp("llvm-native",optarg,11) == 0) {
                    bType = nativeBuilder;
                } else if (strncmp("llvm-jit",optarg,8) == 0) {
                    bType = jitBuilder;
                } else if (strncmp("model", optarg, 4) == 0) {
                    bType = modelBuilder;
                } else {
                    cerr << "Unknown builder: " << optarg << endl;
                    exit(1);
                }
                break;
            case 'b':
                if (!*optarg) {
                    cerr << "Bad builder options, use the form: foo=bar,baz=bip"
                         << endl;
                    exit(1);
                }
                subopts = optarg;
                while (*subopts != '\0') {
                    switch (getsubopt(&subopts, token, &value)) {
                        default:
#ifdef __APPLE__
                        // OSX getsubopt does the split on = for us.
                        // it puts the key in extern suboptarg, and the
                        // possible value in value
                        if (value) {
                            crack.options->optionMap[suboptarg] = value;
                        }
                        else {
                            crack.options->optionMap[suboptarg] = "true";
                        }
#else
                        string v(value);
                        string::size_type pos;
                        if ((pos = v.find('=')) != string::npos) {
                            // as key,val
                            crack.options->optionMap[v.substr(0,pos)] =
                                    v.substr(pos+1);
                        }
                        else {
                            // as bool
                            crack.options->optionMap[v] = "true";
                        }
#endif
                    }
                }
                break;
            case 'd':
                crack.options->dumpMode = true;
                break;
            case 'C':
                crack.cacheMode = true;
                break;
            case 'K':
                crack.cacheMode = false;
                break;
            case 'h':
                usage(0);
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
            case 'v':
                crack.options->verbosity++;
                break;
            case 'q':
                crack.options->verbosity = 0;
                crack.options->quiet = true;
                break;
            case 'n':
                crack.noBootstrap = true;
                break;
            case 'G':
                crack.useGlobalLibs = false;
                break;
            case 'm':
                crack.migrationWarnings = true;
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
            case doubleBuilder:
                useDoubleBuilder = true;
                break;
            case dumpFuncTable:
                doDumpFuncTable = true;
                break;
            case 't':
                if (!Tracer::parse(optarg))
                    exit(1);
                break;
        }
    }
    
    // check for options errors
    if (optionsError)
        usage(1);

    if (bType == jitBuilder) {
        // immediate execution in JIT
        crack.setBuilder(new builder::mvll::LLVMJitBuilder());
        if (useDoubleBuilder)
            crack.setCompileTimeBuilder(new builder::mvll::LLVMJitBuilder());
    }
    else {
        if (bType == nativeBuilder) {
            // Disable cache mode (doesn't work for native builders).
            crack.cacheMode = false;

            // compile to native binary
            crack.setBuilder(new builder::mvll::LLVMLinkerBuilder());
        } else {
            crack.setBuilder(new builder::mdl::ModelBuilder());
        }
        crack.setCompileTimeBuilder(new builder::mvll::LLVMJitBuilder());
    }

    if (!libPath.empty())
        crack.addToSourceLibPath(libPath);

    // are there any more arguments?
    if (optind == argc) {
        cerr << "You need to define a script or the '-' option to read "
                "from standard input." << endl;
        rc = -1;
    } else if (!strcmp(argv[optind], "-")) {
        crack.setArgv(argc - optind, &argv[optind]);
        // ensure a reasonable output file name in native mode
        if (bType == nativeBuilder &&
            crack.options->optionMap.find("out") ==
              crack.options->optionMap.end()) {
            crack.options->optionMap["out"] = "crack_output";
        }
        rc = crack.runScript(cin, "<stdin>", true);
    } else {
        // it's the script name - run it.
        ifstream src(argv[optind]);
        if (!src.good()) {
            cerr << "Unable to open: " << argv[optind] << endl;
            rc = -1;
        } else {
            crack.setArgv(argc - optind, &argv[optind]);
            rc = crack.runScript(src, argv[optind], false);
        }
    }
    
    if (bType == jitBuilder && !crack.options->dumpMode)
        crack.callModuleDestructors();

    if (crack.options->statsMode) {
        crack.printStats(cerr);
    }
    
    if (doDumpFuncTable)
        crack::debug::dumpFuncTable(cerr);

    return rc;

}
