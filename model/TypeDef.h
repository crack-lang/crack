
#ifndef _model_TypeDef_h
#define _model_TypeDef_h

#include <spug/RCPtr.h>

#include "Def.h"

namespace model {

SPUG_RCPTR(Context);

// a type.
class TypeDef : public Def {
    public:
        
        // the type's context - contains all of the method/attribute 
        // definitions for the type.
        ContextPtr context;
        
        TypeDef(const char *name) : Def(name) {}
};

} // namespace model


#endif
