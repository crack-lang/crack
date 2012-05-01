// Copyright 2012 Google Inc.

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
