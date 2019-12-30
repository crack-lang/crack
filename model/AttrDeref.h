// Copyright 2019 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_AttrDeref_h_
#define _model_AttrDeref_h_

#include "Deref.h"

#include "FuncCall.h"
#include "FuncDef.h"
#include "OverloadDef.h"

namespace model {

SPUG_RCPTR(AttrDeref);

/**
 * Attribute derference.  This is a "virtual dereference" for attributes
 * that exist by virtual of the presence of an "oper set" method.
 *
 * If "oper set" exists for an attribute, we create one of these to expand the
 * setter or getter later on.
 */
class AttrDeref : public Deref {
    protected:
        virtual ExprPtr createDeref(Context &context) const;

    public:
        FuncDefPtr operGet;
        OverloadDefPtr operSet;

        AttrDeref(Expr *receiver, OverloadDef *operSet, FuncDef *operGet,
                  TypeDef *noGetterType);
        virtual ResultExprPtr emit(Context &context);
        virtual bool isProductive() const { return true; }
        virtual bool isVolatile() const { return false; }
        virtual void writeTo(std::ostream &out) const;
        virtual ExprPtr convert(Context &context, TypeDef *type);
        virtual ExprPtr makeAssignment(Context &context, Expr *val);
};

}

#endif
