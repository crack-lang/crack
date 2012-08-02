// Copyright 2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "BModuleDef.h"

#include "model/EphemeralImportDef.h"

using namespace model;
using namespace builder::mvll;

void BModuleDef::recordDependency(ModuleDef *other) {

    // quit if the dependency already is recorded
    for (VarDefVec::iterator depi = orderedForCache.begin();
         depi != orderedForCache.end();
         ++depi
         ) {
        EphemeralImportDef *modDef;
        if ((modDef = EphemeralImportDefPtr::rcast(*depi)) &&
             modDef->module == other
            )
            return;
    }

    EphemeralImportDefPtr def = new EphemeralImportDef(other);
    def->setOwner(this);
    orderedForCache.push_back(def.get());
}