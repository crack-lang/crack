
#ifndef _model_InstVarDef_h_
#define _model_InstVarDef_h_

#include <vector>
#include "VarDef.h"

namespace model {

SPUG_RCPTR(Expr);

SPUG_RCPTR(InstVarDef);

/**
 * An instance variable definition - these are special because we preserve the 
 * initializer.
 */
class InstVarDef : public VarDef {
    public:
        ExprPtr initializer;

        InstVarDef(TypeDef *type, const std::string &name,
                   Expr *initializer
                   ) :
            VarDef(type, name),
            initializer(initializer) {
        }
};

} // namespace model

#endif
