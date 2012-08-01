// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
            (loc) ? loc.getLineNumber() : 0,
            llvm::DIType(),
            false, // local to unit (i.e. like C static)
            true, // is definition,
            0 // scope line
    );

}

MDNode* DebugInfo::emitLexicalBlock(const parser::Location &loc) {

    currentScope = builder.createLexicalBlock(currentScope,
                                              currentFile,
                                              (loc) ? loc.getLineNumber() : 0,
                                              0 // col
                                              );

}
