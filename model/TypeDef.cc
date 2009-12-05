
#include "TypeDef.h"

#include "builder/Builder.h"
#include "VarDefImpl.h"
#include "Context.h"
#include "VarDef.h"

using namespace model;

VarDefPtr TypeDef::emitVarDef(Context &container, const std::string &name,
                               Expr *initializer
                               ) {
    return container.builder.emitVarDef(container, this, name, initializer);
}

bool TypeDef::matches(const TypeDef &other) {
    // XXX need to deal with derived classes and possibly equivalent types 
    // with a different identity.
    return &other == this;
}    

bool TypeDef::emitNarrower(TypeDef &target) {
    assert(context);
    
    // if the types are the same, we're done.
    if (this == &target)
        return true;
    
    // look through the parents
    int i = 0;
    for (Context::ContextVec::iterator iter = context->parents.begin();
         iter != context->parents.end();
         ++iter, ++i
         ) {
        assert((*iter)->returnType && 
               "Parent context is not bound to a type."
               );
        TypeDef &baseType = *(*iter)->returnType;
        if (baseType.emitNarrower(target)) {
            // have the builder actually emit tha narrower code to obtain an 
            // expression of the base type from this type.
            context->builder.emitNarrower(*this, baseType, i);
            return true;
        }
    }

    // the target type was not found in our ancestry.
    return false;
}
