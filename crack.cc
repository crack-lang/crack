
#include <iostream>
#include <fstream>
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include "builder/LLVMBuilder.h"
#include "Crack.h"

using namespace std;

bool dump = false;

int main(int argc, const char **argv) {
    if (argc < 2) {
        cerr << "Usage:" << endl;
        cerr << "  crack <script>" << endl;
        return 1;
    }
    
    // parse the main module
    const char **arg = &argv[1];
    while (*arg) {
        if (!strcmp(*arg, "-")) {
            Crack::getInstance().runScript(cin, "<stdin>");
            break;
        } else if (!strcmp(*arg, "-d")) {
            Crack::getInstance().dump = true;
        } else if (!strcmp(*arg, "-l")) {
            ++arg;
            Crack::getInstance().addToSourceLibPath(*arg);
        } else {
            ifstream src(*arg);
            Crack::getInstance().runScript(src, *arg);
            break;
        }
        ++arg;
    }
}
