
#ifndef _model_VarRef_h_
#define _model_VarRef_h_

#include "Expr.h"

namespace model {

SPUG_RCPTR(VarDef);

SPUG_RCPTR(VarRef);

// A variable reference - or dereference, actually.
class VarRef : public Expr {
    public:
        // the definition of the variable we're referencing
        VarDefPtr def;
        VarRef(const VarDefPtr &def);
        
        virtual void emit(Context &context);
};

} // namespace model

#endif
