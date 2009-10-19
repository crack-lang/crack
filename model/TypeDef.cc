
#include "TypeDef.h"

#include "builder/Builder.h"
#include "BuilderVarDefData.h"
#include "Context.h"
#include "VarDef.h"

using namespace model;

VarDefPtr TypeDef::emitVarDef(Context &container, const std::string &name,
                               const ExprPtr &initializer
                               ) {
    return container.builder.emitVarDef(container, this, name, initializer);
}

bool TypeDef::matches(const TypeDef &other) {
    // XXX need to deal with derived classes and possibly equivalent types 
    // with a different identity.
    return &other == this;
}    
