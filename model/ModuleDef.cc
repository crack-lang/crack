// Copyright 2010 Google Inc.

#include "ModuleDef.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

ModuleDef::ModuleDef(const std::string &name, Namespace *parent) :
    VarDef(0, name),
    Namespace(name),
    parent(parent),
    finished(false) {
}

bool ModuleDef::hasInstSlot() {
    return false;
}

void ModuleDef::close(Context &context) {
    context.builder.closeModule(context, this);
}

NamespacePtr ModuleDef::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}
