// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleDef.h"

#include "spug/check.h"
#include "builder/Builder.h"
#include "util/SourceDigest.h"
#include "Context.h"
#include "Deserializer.h"
#include "ModuleDefMap.h"
#include "Serializer.h"

using namespace std;
using namespace model;
using namespace crack::util;

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
    StatState sState(&context, ConstructStats::builder, this);
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

#define CRACK_METADATA_V1 2271218416

void ModuleDef::serialize(Serializer &serializer) const {
    serializer.module = this;
    serializer.write(CRACK_METADATA_V1, "magic");

    // XXX we need to write the source hash.

    // calculate the dependencies
    ModuleDefMap deps;
    for (VarDefMap::const_iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         )
        iter->second->addDependenciesTo(this, deps);

    // make sure we have the imports (we can import a module without
    // incorporating any of its defs)
    for (vector<ModuleDefPtr>::const_iterator iter = imports.begin();
         iter != imports.end();
         ++iter
         )
        deps[(*iter)->getFullName()] = *iter;

    // write the dependencies
    serializer.write(deps.size(), "#deps");
    for (ModuleDefMap::const_iterator iter = deps.begin(); iter != deps.end();
         ++iter
         ) {
        serializer.write(iter->first, "canonicalName");
        serializer.write(iter->second->getDefHash(), "hashVal");
    }

    // write all of the symbols
    Namespace::serializeDefs(serializer);
}

bool ModuleDef::readHeaderAndVerify(Deserializer &deser,
                                    const SourceDigest &digest
                                    ) {
    if (deser.readUInt("magic") != CRACK_METADATA_V1)
        return false;

    //deser.readBlob()  // XXX read the source hash.

    // read and load the dependencies
    int count = deser.readUInt("#deps");
    for (int i = 0; i < count; ++i) {
        ModuleDefPtr mod =
            deser.context->construct->getModule(
                deser.readString(64, "canonicalName")
            );

        // if the dependency has a different definition hash from what we were
        // built against, we have to recompile.
        if (mod->getDefHash() != deser.readUInt("hashVal"))
            return false;
    }

    return true;
}

ModuleDefPtr ModuleDef::deserialize(Deserializer &deser,
                                    const string &canonicalName
                                    ) {
    ModuleDefPtr mod =
        deser.context->builder.materializeModule(*deser.context, canonicalName,
                                                 0 // owner
                                                 );
    deser.context->ns = mod.get();
    mod->deserializeDefs(deser);
    return mod;
}

