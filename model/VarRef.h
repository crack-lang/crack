// Copyright 2009-2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
