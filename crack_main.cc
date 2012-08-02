// Copyright 2009-2011 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include <iostream>
#include <fstream>
#include <getopt.h>
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "builder/BuilderOptions.h"
#include "builder/llvm/LLVMJitBuilder.h"
#include "builder/llvm/LLVMLinkerBuilder.h"
#include "Crack.h"
#include "crack_config.h"

using namespace std;

typedef enum {
    jitBuilder,
    nativeBuilder,
    doubleBuilder = 1001
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
    {"no-cache", false, 0, 'C'},
    {"no-bootstrap", false, 0, 'n'},
    {"no-default-paths", false, 0, 'G'},
    {"migration-warnings", false, 0, 'm'},
    {"lib", true, 0, 'l'},
    {"version", false, 0, 0},
    {"stats", false, 0, 0},
    {0, 0, 0, 0}
};

static std::string prog;

void version() {
    cout << prog << " " << CRACK_VERSION_STRING << endl;
}

void usage(int retval) {
    version();
    cout << "Usage:" << endl;
    cout << "  " << prog << " [options] <source file>" << endl;
    cout << " -B <name>  --builder            Main builder to use (llvm-jit or"
            " llvm-native)" << endl;
    cout << " -b <opts>  --builder-opts       Builder options in the form "
            "foo=bar,baz=bip" << endl;
    cout << " -d         --dump               Dump IR to stdout instead of "
            "running or compiling" << endl;
    cout << " --double-builder                Run multiple internal builders, "
            "even in JIT mode." << endl;
    cout << " -G         --no-default-paths   Do not include default module"
            " search paths" << endl;

    /*
    cout << " -C         --no-cache           Do not cache or use cached modules"
            << endl;
    */
    cout << " -C                              Turn on module caching (unfinished, not fully working)" << endl;

    cout << " -g         --debug              Generate DWARF debug information"
            << endl;
    cout << " -O <N>     --optimize N         Use optimization level N (default"
            " 2)" << endl;
    cout << " -l <path>  --lib                Add directory to module search "
            "path" << endl;
    cout << " -m         --migration-warnings Include migration warnings"
            << endl;
    cout << " -n         --no-bootstrap       Do not load bootstrapping modules"
            << endl;
    cout << " -v         --verbose            Verbose output, use more than once"
            " for greater effect" << endl;
    cout << " -q         --quiet              No extra output, implies"
            " verbose level 0" << endl;
    cout << "            --version            Emit the version number and exit"
            << endl;
    cout << "            --stats              Emit statistics about compile "
            "time operations." << endl;
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

    // parse the main module
    int opt, idx;
    char *subopts, *value;
    char * const token[] = { NULL };
    bool optionsError = false;
    bool useDoubleBuilder = false;    
    while ((opt = getopt_long(argc, argv, "+B:b:dgO:nCGml:vq", longopts, &idx)) !=
           -1) {
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
                if (strncmp("llvm-native",optarg,11) == 0)
                    bType = nativeBuilder;
                else if (strncmp("llvm-jit",optarg,8) == 0)
                    bType = jitBuilder;
                else {
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
                    }
                }
                break;
            case 'd':
                crack.options->dumpMode = true;
                break;
            case 'C':
                crack.options->cacheMode = true;
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
            case doubleBuilder:
                useDoubleBuilder = true;
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
        rc = -1;
    } else if (!strcmp(argv[optind], "-")) {
        crack.setArgv(argc - optind, &argv[optind]);
        // ensure a reasonable output file name in native mode
        if (bType == nativeBuilder &&
            crack.options->optionMap.find("out") ==
              crack.options->optionMap.end()) {
            crack.options->optionMap["out"] = "crack_output";
        }
        rc = crack.runScript(cin, "<stdin>");
    } else {
        // it's the script name - run it.
        ifstream src(argv[optind]);
        if (!src.good()) {
            cerr << "Unable to open: " << argv[optind] << endl;
            rc = -1;
        }
        else {
            crack.setArgv(argc - optind, &argv[optind]);
            rc = crack.runScript(src, argv[optind]);
        }
    }
    
    if (bType == jitBuilder && !crack.options->dumpMode)
        crack.callModuleDestructors();

    if (crack.options->statsMode) {
        crack.printStats(cerr);
    }

    return rc;

}
