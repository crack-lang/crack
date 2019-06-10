// Copyright 2019 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LazyImports.h"

#include "spug/check.h"
#include "spug/stlutil.h"
#include "Deserializer.h"
#include "ModuleDef.h"
#include "Serializer.h"
#include "TypeDef.h"      // Required for RCPtr resolution.

using namespace model;
using namespace std;

void LazyImports::addImportTo(ModuleImports &imports,
                              const ImportedDef &def
                              ) {
    imports.addImport(def);
    byLocalName.emplace(def.local, &imports);
}

void LazyImports::addImport(const string &moduleName,
                            bool rawSharedLib,
                            const ImportedDef &def
                            ) {
    ImportsByModule::iterator iter =
        byModule.find(make_pair(moduleName, rawSharedLib));
    if (iter != byModule.end()) {
        addImportTo(*iter->second, def);
        return;
    }

    ModuleDef::StringVec parsedModName =
        ModuleDef::parseCanonicalName(moduleName);
    ModuleImports *imports = new ModuleImports(parsedModName,
                                               rawSharedLib
                                               );
    byModule.emplace(make_pair(moduleName, rawSharedLib), imports);
    addImportTo(*imports, def);
}

LazyImports::~LazyImports() {
    SPUG_FOR(ImportsByModule, iter, byModule)
        delete iter->second;
}

LazyImports::ModuleImports LazyImports::getImport(const string &localName) {
    ImportsMap::iterator iter = byLocalName.find(localName);
    if (iter == byLocalName.end())
        return ModuleImports();

    SPUG_FOR(ImportedDefVec, defIter, iter->second->getImports()) {
        if (defIter->local == localName)
            return ModuleImports(iter->second->getModuleName(),
                                 iter->second->isRawSharedLib(),
                                 *defIter
                                 );
    }
    SPUG_CHECK(false, "lazy import not found under registered name!");
}

void LazyImports::serialize(Serializer &serializer) {
}

LazyImportsPtr LazyImports::deserialize(Deserializer &deser) {
    return 0;
}
