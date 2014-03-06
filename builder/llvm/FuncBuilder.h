// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_llvm_FuncBuilder_h_
#define _builder_llvm_FuncBuilder_h_

#include "BTypeDef.h"
#include "BFuncDef.h"

#include "model/Context.h"
#include "model/FuncDef.h"
#include "model/ArgDef.h"

#include <vector>
#include <string>
#include <llvm/IR/Function.h>

namespace model {
    class TypeDef;
}

namespace builder {
namespace mvll {

class FuncBuilder {
public:
    model::Context &context;
    BTypeDefPtr returnType;
    BTypeDefPtr receiverType;
    BFuncDefPtr funcDef;
    int argIndex;
    llvm::Function::LinkageTypes linkage;

    // the receiver variable
    model::VarDefPtr receiver;

    // This is lame:  if there is a receiver, "context"
    // should be the function context (and include a definition for
    // the receiver) and the finish() method should be called with a
    // "false" value - indicating that the definition should not be
    // stored in the context.
    // If there is no receiver, it's safe to call this with the
    // context in which the definition should be stored.
    // @param addr an external C function address if specified.
    FuncBuilder(model::Context &context, model::FuncDef::Flags flags,
                BTypeDef *returnType,
                const std::string &name,
                size_t argCount,
                llvm::Function::LinkageTypes linkage =
                    llvm::Function::ExternalLinkage,
                void *addr = 0
                );

    void finish(bool storeDef = true);

    void setSymbolName(const std::string &sname) {
        assert(funcDef);
        funcDef->symbolName = sname;
    }

    void addArg(const char *name, model::TypeDef *type);

    void setArgs(const std::vector<model::ArgDefPtr> &args);

    void setReceiverType(BTypeDef *type) {
        funcDef->receiverType = type;
        receiverType = type;
    }

};


} // end namespace builder::vmll
} // end namespace builder

#endif
