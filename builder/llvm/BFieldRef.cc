// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

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

