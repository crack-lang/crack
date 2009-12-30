
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
        VarRef(VarDef *def);
        
        virtual ResultExprPtr emit(Context &context);
        
        // variable references are non-productive, so we override.
        virtual bool isProductive() const;
        virtual void writeTo(std::ostream &out) const;
};

} // namespace model

#endif
