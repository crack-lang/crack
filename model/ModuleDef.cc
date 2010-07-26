// Copyright 2010 Google Inc.

#include "ModuleDef.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

ModuleDef::ModuleDef(const std::string &name, Context *moduleContext) :
    VarDef(0, name),
    moduleContext(moduleContext) {
}

VarDefPtr ModuleDef::lookUp(const std::string &name) {
    return moduleContext->ns->lookUp(name);
}

bool ModuleDef::hasInstSlot() {
    return false;
}

void ModuleDef::close() {
    moduleContext->builder.closeModule(*moduleContext, this);
}
