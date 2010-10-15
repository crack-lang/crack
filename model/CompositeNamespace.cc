// Copyright 2009 Google Inc.

#include "CompositeNamespace.h"

#include "VarDef.h"

using namespace model;

CompositeNamespace::CompositeNamespace(Namespace *parent0, 
                                       Namespace *parent1
                                       ) :
        Namespace(parent0->getName()),
        parents(2) {
    parents[0] = parent0;
    parents[1] = parent1;
}

NamespacePtr CompositeNamespace::getParent(unsigned index) {
    if (index < parents.size())
        return parents[index];
    else
        return 0;
}

void CompositeNamespace::addDef(VarDef *def) {
    assert(parents.size());
    parents[0]->addDef(def);
}

void CompositeNamespace::removeDef(VarDef *def) {
    assert(parents.size());
    parents[0]->removeDef(def);
}

void CompositeNamespace::addAlias(VarDef *def) {
    assert(parents.size());
    parents[0]->addAlias(def);
}

void CompositeNamespace::addAlias(const std::string &name, VarDef *def) {
    assert(parents.size());
    parents[0]->addAlias(name, def);
}

void CompositeNamespace::replaceDef(VarDef *def) {
    assert(parents.size());
    parents[0]->replaceDef(def);
}

