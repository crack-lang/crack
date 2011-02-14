// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFieldRef.h"

#include "model/VarDef.h"
#include "BTypeDef.h"
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

    unsigned index = BInstVarDefImplPtr::rcast(def->impl)->index;

    // if the variable is from a complete type, we can emit it.
    //  Otherwise, we need to store a placeholder.
    BTypeDef *owner = BTypeDefPtr::acast(def->getOwner());
    if (owner->complete) {
        Value *fieldPtr =
                bb.builder.CreateStructGEP(bb.lastValue, index);
        bb.lastValue = bb.builder.CreateLoad(fieldPtr);
    } else {
        // create a fixup object for the reference
        BTypeDef *typeDef =
                BTypeDefPtr::rcast(def->type);

        // stash the aggregate, emit a placeholder for the
        // reference
        Value *aggregate = bb.lastValue;
        PlaceholderInstruction *placeholder =
                new IncompleteInstVarRef(typeDef->rep, aggregate,
                                         index,
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

