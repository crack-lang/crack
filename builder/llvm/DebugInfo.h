// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

// a class for maintaining debug information. in llvm, this consists
// of metadeta that is emitted into the ir

#ifndef _builder_llvm_DebugInfo_h_
#define _builder_llvm_DebugInfo_h_

#include "parser/Location.h"
#include "BTypeDef.h"
#include <string>
#include <llvm/DebugInfo.h>
#include <llvm/DIBuilder.h>

namespace llvm {
    class Module;
}

namespace model {
    class TypeDef;
}

namespace builder {

class BuilderOptions;

namespace mvll {

class DebugInfo {

private:
    llvm::Module *module;
    llvm::DIBuilder builder;
    llvm::DIFile currentFile;
    llvm::DIDescriptor currentScope;

public:

    static const unsigned CRACK_LANG_ID = llvm::dwarf::DW_LANG_lo_user + 50;

    DebugInfo(llvm::Module *m,
              const std::string &file,
              const std::string &path,
              const BuilderOptions *options
              );

    void emitFunctionDef(const std::string &name,
                         const parser::Location &loc
                         );

    llvm::MDNode* emitLexicalBlock(const parser::Location &loc);

    void createBasicType(BTypeDef *type,
                         int sizeInBits,
                         unsigned encoding
                         );

    void declareLocal(const BTypeDef *type,
                      llvm::Value *var,
                      llvm::BasicBlock *instr,
                      const parser::Location *loc
                      );

    void addDebugLoc(llvm::Instruction *instr,
                     const parser::Location *loc
                     );

};

} // end namespace builder::vmll
} // end namespace builder

#endif
