// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>

#include "DebugInfo.h"

#include <llvm/Module.h>

using namespace llvm;
using namespace std;
using namespace builder::mvll;

DebugInfo::DebugInfo(Module *m,
                     const string &file
                     ): module(m),
                        debugFactory(*m),
                        compileUnit(debugFactory.CreateCompileUnit(
                                dwarf::DW_LANG_lo_user,
                                file,
                                "./",
                                "crack",
                                false, // isMain
                                false, // isOptimized
                                "" // flags
                                )),
                        currentScope(compileUnit) { }

void DebugInfo::emitFunctionDef(const std::string &name,
                                const parser::Location &loc) {

    currentScope = debugFactory.CreateSubprogram(
            currentScope,
            name,
            name,
            name,
            compileUnit,
            loc.getLineNumber(),
            llvm::DIType(),
            false, // local to unit (i.e. like C static)
            true // is definition
    );

}

MDNode* DebugInfo::emitLocation(const parser::Location &loc) {

    DIDescriptor d = debugFactory.CreateLocation(loc.getLineNumber(),
                                0, // col
                                currentScope
                                );
    return d.getNode();

}

MDNode* DebugInfo::emitLexicalBlock(const parser::Location &loc) {

    currentScope = debugFactory.CreateLexicalBlock(currentScope,
                                                   loc.getLineNumber(),
                                                   0 // col
                                                   );

}
