// Copyright 2010,2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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

bool CompositeNamespace::isHiddenScope() {
    return parents[0]->isHiddenScope();
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

OverloadDefPtr CompositeNamespace::replaceDef(VarDef *def) {
    assert(false && "composite namespace mutation replaceDef called");
}

