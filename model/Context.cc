
#include "Context.h"

#include "model/BuilderContextData.h"
#include "model/OverloadDef.h"
#include "model/VarDefImpl.h"
#include "builder/Builder.h"
#include "VarDef.h"
#include "StrConst.h"
#include "TypeDef.h"

using namespace model;
using namespace std;

Context::Context(builder::Builder &builder, Context::Scope scope,
                 Context *parentContext
                 ) :
    builder(builder),
    parent(parentContext),
    scope(scope),
    complete(false),
    returnType(parentContext ? parentContext->returnType : TypeDefPtr(0)),
    globalData(parentContext ? parentContext->globalData : new GlobalData()) {
}

ContextPtr Context::createSubContext(Scope newScope) {
    return new Context(builder, newScope, this);
}

void Context::createModule(const char *name) {
    builder.createModule(name);
}

namespace {
    inline void allocOverload(OverloadDefPtr &overload, const string &name) {
        if (!overload)
            overload = new OverloadDef(name);
    }
}

OverloadDefPtr Context::aggregateOverloads(const std::string &varName) {
    // see if this is defined locally
    VarDefPtr var = lookUp(varName);
    FuncDef *func = 0;
    if (var) {
        // if it's already an overload, we're done
        OverloadDef *overload = dynamic_cast<OverloadDef *>(var.obj);
        if (overload)
            return overload;
        
        // make sure it's at least a function
        func = dynamic_cast<FuncDef *>(var.obj);
        if (!func)
            return 0;
    }

    OverloadDefPtr overloads;
    if (parent)
        overloads = parent->aggregateOverloads(varName);

    // if we got a local override, add it to the overloads
    if (func) {
        // make sure we've got one
        allocOverload(overloads, varName);
        overloads->funcs.push_back(func);
    }
    
    // this could be overloads or null
    return overloads;
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

FuncDefPtr Context::lookUp(const std::string &varName,
                           const vector<ExprPtr> &args
                           ) {
    OverloadDefPtr overload = aggregateOverloads(varName);
    if (!overload)
        return 0;
    return overload->getMatch(args);
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

