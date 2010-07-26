// Copyright 2009 Google Inc.

#include "LocalNamespace.h"

#include "VarDef.h"

using namespace model;

NamespacePtr LocalNamespace::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}
