// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
