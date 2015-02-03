// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_ModuleMerger_h_
#define _builder_llvm_ModuleMerger_h_

#include <map>
#include <string>

#include <llvm/Transforms/Utils/ValueMapper.h>

#include "spug/Tracer.h"

namespace llvm {
    class ExecutionEngine;
    class Function;
    class GlobalValue;
    class GlobalVariable;
    class Module;
    class Value;
}

namespace builder { namespace mvll {

// This is a lightweight version of the LLVM linker tailored for Crack's very
// specific needs.  The most important difference is that it doesn't do type
// translation because Crack relies on globally unique type objects.
// It also doesn't try to do things that the Crack language doesn't support
// (like global appending).
class ModuleMerger {
    private:

        // the target module that we are merging everything into
        llvm::Module *target;

        // maps a value in a source module to the corresponding value in
        // 'target'.
        llvm::ValueToValueMapTy valueMap;

        // optional execution engine
        llvm::ExecutionEngine *execEng;

        // The rep id of the merged module.
        int mergedModuleRepId;

        // Tracing support.
        static spug::Tracer tracer;

        // Returns true if the global is already defined in 'target'.
        bool defined(llvm::GlobalValue *gval);

        // Copy global variable attributes and alignment from src to dst.
        void copyGlobalAttrs(llvm::GlobalValue *dst, llvm::GlobalValue *src);

        // Functions called from the top-level to add different kinds of
        // entitiies to the target.
        void addGlobalDeclaration(llvm::GlobalVariable *gvar);
        void addFunctionDeclaration(llvm::Function *func);
        void addInitializer(llvm::GlobalVariable *gvar);
        void addFunctionBody(llvm::Function *func);

    public:
        static bool trace;

        // name: the target module name.
        ModuleMerger(const std::string &name, int mergedModuleRepId,
                     llvm::ExecutionEngine *execEng = 0
                     );

        // Merge 'module' into the target.
        void merge(llvm::Module *module);

        // Returns the target module (that we've merged all of the other
        // modules into).
        llvm::Module *getTarget() const { return target; }

        // Returns the rep id of the merged module.
        int getRepId() const { return mergedModuleRepId; }
};

}} // namespace builder::mvll

#endif
