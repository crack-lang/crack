// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
        
        virtual bool isConstant() { return true; }

        virtual void serialize(Serializer &serializer, bool writeKind,
                               const Namespace *ns
                               ) const;
        
        static ConstVarDefPtr deserialize(Deserializer &deser);
};

} // namespace model

#endif
