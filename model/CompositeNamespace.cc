// Copyright 2009 Google Inc.

#include "CompositeNamespace.h"

#include "Expr.h"
#include "VarDef.h"
#include "ModuleDef.h"
#include "OverloadDef.h"

using namespace model;

CompositeNamespace::CompositeNamespace(Namespace *parent0, 
                                       Namespace *parent1
                                       ) :
        Namespace(parent0->getNamespaceName()),
        parents(2) {
    parents[0] = parent0;
    parents[1] = parent1;
}

ModuleDefPtr CompositeNamespace::getModule() {
    return parents[0]->getModule();
}

NamespacePtr CompositeNamespace::getParent(unsigned index) {
    if (index < parents.size())
        return parents[index];
    else
        return 0;
}

void CompositeNamespace::addDef(VarDef *def) {
    assert(OverloadDefPtr::cast(def) && 
           "composite namespace addDef called on non-overload");
    Namespace::addDef(def);
}

void CompositeNamespace::removeDef(VarDef *def) {
    assert(false && "composite namespace mutation removeDef called");
}

void CompositeNamespace::addAlias(VarDef *def) {
    assert(false && "composite namespace mutation addAlias(d) called");
}

OverloadDefPtr CompositeNamespace::addAlias(const std::string &name, VarDef *def) {
    assert(false && "composite namespace mutation addAlias(n,d) called");
}

void CompositeNamespace::addUnsafeAlias(const std::string &name, VarDef *def) {
    assert(false && "composite namespace mutation addUnsafeAlias(n,d) called");
}

void CompositeNamespace::replaceDef(VarDef *def) {
    assert(false && "composite namespace mutation replaceDef called");
}

