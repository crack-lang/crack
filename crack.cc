
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/VarDefImpl.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include <builder/LLVMBuilder.h>
#include <iostream>
#include <fstream>

using namespace std;

void compileAndRun(istream &src, const char *name) {

    // create the builder and top-level context
    builder::LLVMBuilder builder;
    model::ContextPtr ctx =
        new model::Context(builder, model::Context::module);
    ctx->returnType = ctx->globalData->voidType;
    
    // create the main module, register all of the basic stuff
    ctx->createModule("main");
    ctx->builder.registerPrimFuncs(*ctx);

    parser::Toker toker(src, name);
    parser::Parser parser(toker, ctx);
    
    try {
        parser.parse();

        // close it and run
        ctx->builder.closeModule();
        builder.run();
    } catch (const parser::ParseError &ex) {
        cerr << ex << endl;
    }
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        cerr << "Usage:" << endl;
        cerr << "  crack <script>" << endl;
        return 1;
    }
    
    // parse the main module
    if (!strcmp(argv[1], "-")) {
        compileAndRun(cin, "<stdin>");
    } else {
        ifstream src(argv[1]);
        compileAndRun(src, argv[1]);
    }
}
