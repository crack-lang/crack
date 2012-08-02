// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>

#include "DebugInfo.h"
#include "crack_config.h"
#include "builder/BuilderOptions.h"

#include <llvm/Module.h>
#include <llvm/Support/Dwarf.h>

using namespace llvm;
using namespace std;
using namespace model;
using namespace builder::mvll;

DebugInfo::DebugInfo(Module *m,
                     const string &file,
                     const string &path,
                     const builder::BuilderOptions *options
                     ) :
    module(m),
    builder(*m) {

    builder.createCompileUnit(
            DebugInfo::CRACK_LANG_ID,
            file,
            path,
            CRACK_VERSION_STRING, // needs real version string
            (options->optimizeLevel > 0), // isOptimized
            "", // commandline flags XXX get from BuilderOptions?
            0 // runtime version for objc?
            );
    currentFile = builder.createFile(file, path);
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
                                              (loc) ? loc.getColNumber() : 0
                                              );

}

void DebugInfo::createBasicType(BTypeDef *type,
                                int sizeInBits,
                                unsigned encoding) {

    assert(!type->debugInfo && "type already had debug info");

    type->debugInfo = builder.createBasicType(type->name,
                                              sizeInBits,
                                              sizeInBits,
                                              encoding);

}

void DebugInfo::declareLocal(const BTypeDef *type,
                             Value *&var,
                             BasicBlock *block,
                             const parser::Location *loc) {

    // XXX until we are creating all types...
    if (!type->debugInfo)
        return;

    if (var->getName().empty())
        return;

    assert(type->debugInfo && "type had no debug info");

    DIVariable varInfo = builder.createLocalVariable(
                dwarf::DW_TAG_auto_variable,
                currentScope,
                var->getName(),
                currentFile,
                (loc) ? loc->getLineNumber() : 0,
                type->debugInfo
                );
    Instruction *Call = builder.insertDeclare(var, varInfo, block);
    Call->setDebugLoc(DebugLoc::get((loc) ? loc->getLineNumber() : 0,
                                    (loc) ? loc->getColNumber() : 0,
                                    currentScope
                                    )
                      );

}
