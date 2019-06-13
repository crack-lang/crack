// Copyright 2019 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "LazyImports.h"

#include <sstream>

#include "spug/check.h"
#include "spug/stlutil.h"
#include "Deserializer.h"
#include "ModuleDef.h"
#include "NestedDeserializer.h"
#include "ProtoBuf.h"
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
    SPUG_FOR(ImportsByModule, iter, byModule) {
        ostringstream temp;
        Serializer sub(serializer, temp);

        // Serialize the module name components.
        SPUG_FOR(vector<string>, nameIter, iter->second->getModuleName()) {
            sub.write(CRACK_PB_KEY(1, string), "module.header");
            sub.write(*nameIter, "module");
        }

        // Serialize the symbols
        SPUG_FOR(ImportedDefVec, impIter, iter->second->getImports()) {
            ostringstream temp2;
            Serializer sub2(sub, temp2);
            sub2.write(CRACK_PB_KEY(1, string), "sourceName.header");
            sub2.write(impIter->source, "sourceName");
            if (impIter->local != impIter->source) {
                sub2.write(CRACK_PB_KEY(2, string), "localName.header");
                sub2.write(impIter->local, "localName");
            }

            sub.write(CRACK_PB_KEY(2, string), "import.header");
            sub.write(temp2.str(), "import");
        }

        serializer.write(CRACK_PB_KEY(2, string), "lazyImports.header");
        serializer.write(temp.str(), "lazyImports");
    }
}

ImportedDef LazyImports::deserializeImportSymbol(Deserializer &deser) {
    string local, source;
    bool gotLocalName = false;
    CRACK_PB_BEGIN(deser, 1024, importSymbol)
        CRACK_PB_FIELD(1, string) {
            source = importSymbolDeser.readString(32, "sourceName");
            break;
        }
        CRACK_PB_FIELD(2, string) {
            local = importSymbolDeser.readString(32, "localName");
            gotLocalName = true;
            break;
        }
    CRACK_PB_END
    if (!gotLocalName)
        local = source;
    return ImportedDef(local, source);
}

void LazyImports::deserializeModuleImports(Deserializer &deser) {
    ModuleImports *imports = 0;
    vector<string> moduleName;
    CRACK_PB_BEGIN(deser, 1024, lazyImports)
        CRACK_PB_FIELD(1, string) {
            // make sure we haven't created the ModuleImports object yet.
            SPUG_CHECK(!imports,
                       "bad serialization format: module name elements after "
                       "import symbols."
                       );
            moduleName.push_back(lazyImportsDeser.readString(32, "module"));
            break;
        }
        CRACK_PB_FIELD(2, string) {
            if (!imports) {
                imports = new ModuleImports(moduleName, false);
                pair<string, bool> modId(
                    ModuleDef::joinName(moduleName),
                    false
                );
                byModule[modId] = imports;
            }
            addImportTo(*imports, deserializeImportSymbol(lazyImportsDeser));
            break;
        }
    CRACK_PB_END
}
