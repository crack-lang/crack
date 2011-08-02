// Copyright 2009 Google Inc.

#include "LocalNamespace.h"

#include "ModuleDef.h"
#include "VarDef.h"

using namespace model;

ModuleDefPtr LocalNamespace::getModule() {
    return parent ? parent->getModule() : ModuleDefPtr(0);
}

NamespacePtr LocalNamespace::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}
