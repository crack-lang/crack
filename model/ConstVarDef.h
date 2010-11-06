// Copyright 2009 Google Inc.

#ifndef _model_ConstVarDef_h_
#define _model_ConstVarDef_h_

#include "VarDef.h"

namespace model {

SPUG_RCPTR(ConstVarDef);

class ConstVarDef : public VarDef {
    public:
        ExprPtr expr;

        ConstVarDef(TypeDef *type, const std::string &name,
                    ExprPtr expr
                    ) :
            VarDef(type, name),
            expr(expr) {
        }
};

} // namespace model

#endif
