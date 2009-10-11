
#include "Context.h"

#include <builder/Builder.h>
#include "Def.h"
#include "StrConst.h"
#include "TypeDef.h"

using namespace model;

Context::Context(builder::Builder &builder, Context::Scope scope) :
    builder(builder),
    parent(0),
    scope(scope),
    globalData(new GlobalData()) {
}

void Context::createModule(const char *name) {
    builder.createModule(name);
}

DefPtr Context::lookUp(const std::string &varName) {
    DefMap::iterator iter = defs.find(varName);
    if (iter != defs.end())
        return iter->second;
    else if (parent)
        return parent->lookUp(varName);
    else
        return 0;
}

void Context::addDef(const DefPtr &def) {
    defs[def->name] = def;
}

StrConstPtr Context::getStrConst(const std::string &value) {
    StrConstTable::iterator iter = globalData->strConstTable.find(value);
    if (iter != globalData->strConstTable.end()) {
        return iter->second;
    } else {
        // create a new one
        StrConstPtr strConst = builder.createStrConst(value);
        globalData->strConstTable[value] = strConst;
        return strConst;
    }
}

