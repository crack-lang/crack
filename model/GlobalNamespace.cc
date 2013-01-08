// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "GlobalNamespace.h"

#include "ModuleDef.h"

using namespace model;

ModuleDefPtr GlobalNamespace::getModule() {
    if (builtin)
        return builtin;
    else
        return LocalNamespace::getModule();
}