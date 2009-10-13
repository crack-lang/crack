
#include "parser/ParseError.h"
#include "parser/Parser.h"
#include "parser/Toker.h"
#include "model/Context.h"
#include "model/TypeDef.h"
#include <builder/LLVMBuilder.h>
#include <iostream>
#include <fstream>

int main(int argc, const char **argv) {
    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        std::cerr << "  crack <script>" << std::endl;
        return 1;
    }
    
    // create the builder and top-level context
    builder::LLVMBuilder builder;
    model::ContextPtr ctx =
        new model::Context(builder, model::Context::module);
    
    // create the main module, register all of the basic stuff
    ctx->createModule("main");
    ctx->builder.registerPrimFuncs(*ctx);

    // parse the main module
    std::ifstream src(argv[1]);
    parser::Toker toker(src, argv[1]);
    parser::Parser parser(toker, ctx);
    
    try {
        parser.parse();

        // close it and run
        ctx->builder.closeModule();
        builder.run();
    } catch (const parser::ParseError &ex) {
        std::cerr << ex << std::endl;
    }

}

