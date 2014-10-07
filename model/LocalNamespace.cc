// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LocalNamespace.h"

#include "ModuleDef.h"
#include "TypeDef.h"
#include "VarDef.h"

using namespace model;

ModuleDefPtr LocalNamespace::getModule() {
    return parent ? parent->getModule() : ModuleDefPtr(0);
}

NamespacePtr LocalNamespace::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}
