
#include "Context.h"

#include "model/BuilderContextData.h"
#include "model/BuilderVarDefData.h"
#include "builder/Builder.h"
#include "VarDef.h"
#include "StrConst.h"
#include "TypeDef.h"

using namespace model;

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext
                 ) :
    builder(builder),
    parent(parentContext),
    scope(scope),
    globalData(parentContext ? parentContext->globalData : new GlobalData()) {
}

ContextPtr Context::createSubContext(Scope newScope) {
    return new Context(builder, newScope, this);
}

void Context::createModule(const char *name) {
    builder.createModule(name);
}

VarDefPtr Context::lookUp(const std::string &varName) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end())
        return iter->second;
    else if (parent)
        return parent->lookUp(varName);
    else
        return 0;
}

void Context::addDef(const VarDefPtr &def) {
    assert(!def->context);
    defs[def->name] = def;
    def->context = this;
}

StrConstPtr Context::getStrConst(const std::string &value) {
    StrConstTable::iterator iter = globalData->strConstTable.find(value);
    if (iter != globalData->strConstTable.end()) {
        return iter->second;
    } else {
        // create a new one
        StrConstPtr strConst = builder.createStrConst(*this, value);
        globalData->strConstTable[value] = strConst;
        return strConst;
    }
}

