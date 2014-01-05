// Copyright 2010 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "BFieldRef.h"

#include "model/VarDef.h"
#include "BTypeDef.h"
#include "VarDefs.h"
#include "BResultExpr.h"
#include "Incompletes.h"
#include "LLVMBuilder.h"
#include "VarDefs.h"

using namespace llvm;
using namespace model;
using namespace builder::mvll;

ResultExprPtr BFieldRef::emit(Context &context) {
    ResultExprPtr aggregateResult = aggregate->emit(context);
    LLVMBuilder &bb = dynamic_cast<LLVMBuilder &>(context.builder);

    // narrow to the ancestor type where the variable is defined.
    bb.narrow(aggregate->type.get(), BTypeDefPtr::acast(def->getOwner()));

    // cast the implementation and type to their local types.
    BFieldDefImplPtr impl = BFieldDefImplPtr::arcast(def->impl);
    BTypeDef *typeDef = BTypeDefPtr::arcast(def->type);

    // if the variable is from a complete type, we can emit it.
    //  Otherwise, we need to store a placeholder.
    BTypeDef *owner = BTypeDefPtr::acast(def->getOwner());
    if (owner->complete) {
        bb.lastValue = impl->emitFieldRef(bb.builder, typeDef->rep,
                                          bb.lastValue
                                          );
    } else {
        // stash the aggregate, emit a placeholder for the
        // reference
        Value *aggregate = bb.lastValue;
        PlaceholderInstruction *placeholder =
            new IncompleteInstVarRef(typeDef->rep, aggregate,
                                     impl.get(),
                                     bb.builder.GetInsertBlock()
                                     );
        bb.lastValue = placeholder;

        // store the placeholder
        owner->addPlaceholder(placeholder);
    }

    // release the aggregate
    aggregateResult->handleTransient(context);

    return new BResultExpr(this, bb.lastValue);
}

Value *BFieldRef::emitAddr(Context &context) const {
    // TODO: refactor duplicate code.
    ResultExprPtr aggregateResult = aggregate->emit(context);
    LLVMBuilder &bb = dynamic_cast<LLVMBuilder &>(context.builder);

    // narrow to the ancestor type where the variable is defined.
    bb.narrow(aggregate->type.get(), BTypeDefPtr::acast(def->getOwner()));

    // cast the implementation and type to their local types.
    BFieldDefImplPtr impl = BFieldDefImplPtr::arcast(def->impl);
    BTypeDef *typeDef = BTypeDefPtr::arcast(def->type);

    // if the variable is from a complete type, we can emit it.
    //  Otherwise, we need to store a placeholder.
    BTypeDef *owner = BTypeDefPtr::acast(def->getOwner());
    Value *addr;
    if (owner->complete) {
        addr = impl->emitFieldAddr(bb.builder, typeDef->rep,
                                   bb.lastValue
                                   );
    } else {
        // stash the aggregate, emit a placeholder for the
        // reference
        Value *aggregate = bb.lastValue;
        PlaceholderInstruction *placeholder =
            new IncompleteInstVarAddr(typeDef->rep, aggregate,
                                      impl.get(),
                                      bb.builder.GetInsertBlock()
                                      );
        addr = placeholder;

        // store the placeholder
        owner->addPlaceholder(placeholder);
    }

    // release the aggregate
    aggregateResult->handleTransient(context);

    return addr;
}

