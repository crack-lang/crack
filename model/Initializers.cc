
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

