
#include "ModuleDef.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

ModuleDef::ModuleDef(const std::string &name, Context *moduleContext) :
    VarDef(0, name),
    moduleContext(moduleContext) {
}

VarDefPtr ModuleDef::lookUp(const std::string &name) {
    return moduleContext->lookUp(name);
}

bool ModuleDef::hasInstSlot() {
    return false;
}

void ModuleDef::create() const {
    moduleContext->createModule(name);
}

void ModuleDef::close() const {
    moduleContext->builder.closeModule();
}
