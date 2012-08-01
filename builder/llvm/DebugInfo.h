// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

// a class for maintaining debug information. in llvm, this consists
// of metadeta that is emitted into the ir

#ifndef _builder_llvm_DebugInfo_h_
#define _builder_llvm_DebufInfo_h_

#include "parser/Location.h"
#include <string>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/DIBuilder.h>

namespace llvm {
    class Module;
}

namespace builder {
namespace mvll {

class DebugInfo {

private:
    llvm::Module *module;
    llvm::DIBuilder builder;
    llvm::DIFile currentFile;
    llvm::DIDescriptor currentScope;

public:

    static const unsigned CRACK_LANG_ID = llvm::dwarf::DW_LANG_lo_user + 50;

    DebugInfo(llvm::Module *m, const std::string &file);

    void emitFunctionDef(const std::string &name,
                         const parser::Location &loc);

    llvm::MDNode* emitLexicalBlock(const parser::Location &loc);

};

} // end namespace builder::vmll
} // end namespace builder

#endif
