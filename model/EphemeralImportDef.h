// Copyright 2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_EphemeralImportDef_h_
#define _model_EphemeralImportDef_h_

#include "VarDef.h"

namespace model {

SPUG_RCPTR(EphemeralImportDef)

/**
 * This is a hack that lets us store an imported ephemeral module in the list
 * of ordered definitions for a module namespace.
 */
class EphemeralImportDef : public VarDef {
    public:
        ModuleDefPtr module;

        EphemeralImportDef(ModuleDef *module) :
            VarDef(0, "ephemeral import: " + module->getNamespaceName()),
            module(module) {
        }
};

} // namespace model

#endif
