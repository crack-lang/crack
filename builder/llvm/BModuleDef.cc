// Copyright 2011 Google Inc

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