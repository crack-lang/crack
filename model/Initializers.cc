// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Initializers.h"

#include "Context.h"
#include "Expr.h"
#include "FuncCall.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;

bool Initializers::addBaseInitializer(TypeDef *base, FuncCall *init) {
    if (baseMap.find(base) != baseMap.end())
        return false;
    baseMap[base] = init;
    return true;
}

FuncCall *Initializers::getBaseInitializer(TypeDef *base) {
    BaseInitMap::iterator iter = baseMap.find(base);
    if (iter == baseMap.end())
        return 0;
    else
        return iter->second.get();
}

bool Initializers::addFieldInitializer(VarDef *var, Expr *init) {
    if (fieldMap.find(var) != fieldMap.end())
        return false;
    fieldMap[var] = init;
    return true;
}

Expr *Initializers::getFieldInitializer(VarDef *field) {
    FieldInitMap::iterator iter = fieldMap.find(field);
    if (iter == fieldMap.end())
        return 0;
    else
        return iter->second.get();
}

