
#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <spug/RCPtr.h>

#include "Def.h"

namespace model {

SPUG_RCPTR(Context);
SPUG_RCPTR(Expr);
SPUG_RCPTR(VarDef);

// a type.
class TypeDef : public Def {
    public:
        
        // the type's context - contains all of the method/attribute 
        // definitions for the type.
        ContextPtr context;
        
        // the default initializer expression (XXX I'm not sure that we want 
        // to keep this, for now it's expedient to be able to do variable 
        // initialization without the whole "oper new" business)
        ExprPtr defaultInitializer;
        
        TypeDef(const char *name) : Def(name) {}
        
        /** Emit a variable definition for the type. */
        VarDefPtr emitVarDef(Context &container, const std::string &name,
                             const ExprPtr &initializer
                             );
};

} // namespace model


#endif
