// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>

#include "DebugInfo.h"

#include <llvm/Module.h>

using namespace llvm;
using namespace std;
using namespace builder::mvll;

DebugInfo::DebugInfo(Module *m,
                     const string &file
                     ): module(m),
                        builder(*m)
                        {
    // XXX
    string dir("./");

    builder.createCompileUnit(
            DebugInfo::CRACK_LANG_ID,
            file,
            dir,
            "crack", // needs real version string
            false, // isOptimized
            "", // flags
            0 // runtime version for objc?
            );
    currentFile = builder.createFile(file, dir);
    currentScope = currentFile;
}

void DebugInfo::emitFunctionDef(const std::string &name,
                                const parser::Location &loc) {

    currentScope = builder.createFunction(
            currentScope,
            name,
            name,
            currentFile,
            loc.getLineNumber(),
            llvm::DIType(),
            false, // local to unit (i.e. like C static)
            true, // is definition,
            0 // scope line
    );

}

MDNode* DebugInfo::emitLexicalBlock(const parser::Location &loc) {

    currentScope = builder.createLexicalBlock(currentScope,
                                              currentFile,
                                              loc.getLineNumber(),
                                              0 // col
                                              );

}
