// Copyright 2010 Google Inc.

#include "ModuleDef.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace model;

ModuleDef::ModuleDef(const std::string &name, Namespace *parent) :
    VarDef(0, name),
    Namespace(name),
    parent(parent),
    finished(false),
    fromExtension(false),
    path() {
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

ModuleDefPtr ModuleDef::getModule() {
    return this;
}

ModuleDef::StringVec ModuleDef::parseCanonicalName(const std::string &name) {
    StringVec result;

    // track the level of bracket nesting, we only split "outer" components.
    int nested = 0;
    int last = 0;

    int i;
    for (i = 0; i < name.size(); ++i) {
        if (!nested) {
            switch (name[i]) {
                case '.':
                    result.push_back(name.substr(last, i - last));
                    last = i;
                    break;
                case '[':
                    ++nested;
                    break;
                case ']':
                    std::cerr << "Invalid canonical name: [" << name << "]" <<
                        std::endl;
                    assert(false);
                    break;
            }
        } else {
            switch (name[i]) {
                case '[':
                    ++nested;
                    break;
                case ']':
                    --nested;
                    break;
            }
        }
    }

    // add the last segment
    result.push_back(name.substr(last, i - last));
    return result;
}