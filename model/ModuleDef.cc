// Copyright 2010 Google Inc.

#include "ModuleDef.h"

#include "builder/Builder.h"
#include "Context.h"

using namespace std;
using namespace model;

ModuleDef::ModuleDef(const std::string &name, Namespace *parent) :
    VarDef(0, name),
    Namespace(name),
    parent(parent),
    finished(false),
    fromExtension(false) {
}

bool ModuleDef::hasInstSlot() {
    return false;
}

bool ModuleDef::matchesSource(const StringVec &libSearchPath) {
    int i;
    string fullSourcePath;
    for (i = 0; i < libSearchPath.size(); ++i) {
        fullSourcePath = libSearchPath[i] + "/" + sourcePath;
        if (Construct::isFile(fullSourcePath))
            break;
    }

    // if we didn't find the source file, assume the module is up-to-date
    // (this will allow us to submit applications as a set of cache files).
    if (i == libSearchPath.size())
        return true;

    return matchesSource(fullSourcePath);
}

void ModuleDef::close(Context &context) {
    ModuleDefPtr lastModule;
    if (context.construct->rootBuilder->options->statsMode) {
        lastModule = context.construct->stats->getCurrentModule();
        context.construct->stats->setCurrentModule(this);
        context.construct->stats->pushState(ConstructStats::builder);
    }
    context.builder.closeModule(context, this);
    if (context.construct->rootBuilder->options->statsMode) {
        context.construct->stats->popState();
        context.construct->stats->setCurrentModule(lastModule);
    }
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
                    last = i + 1;
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