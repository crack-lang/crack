// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Initializers_h_
#define _model_Initializers_h_

#include <map>
#include <spug/RCPtr.h>
#include <spug/RCBase.h>

namespace model {

SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncCall);
class TypeDef;
class VarDef;

SPUG_RCPTR(Initializers);

/**
 * This class keeps track of the list of initializers in a constructor (oper 
 * init).
 */
class Initializers : public spug::RCBase {

    private:
        typedef std::map<TypeDef *, FuncCallPtr> BaseInitMap;
        BaseInitMap baseMap;
        
        typedef std::map<VarDef *, ExprPtr> FieldInitMap;
        FieldInitMap fieldMap;

    public:
        bool addBaseInitializer(TypeDef *base, FuncCall *init);
        FuncCall *getBaseInitializer(TypeDef *base);
        bool addFieldInitializer(VarDef *var, Expr *init);
        Expr *getFieldInitializer(VarDef *var);
};

} // namespace model

#endif
