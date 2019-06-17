// Copyright 2019 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_LazyImports_h_
#define _model_LazyImports_h_

#include <map>
#include <utility>
#include <vector>

#include "spug/RCBase.h"
#include "spug/RCPtr.h"
#include "model/ImportedDef.h"
#include "model/Serializer.h"

namespace model {

SPUG_RCPTR(LazyImports);

/**
 * The complete set of lazy imports for a given context.
 */
class LazyImports : public spug::RCBase {

    public:
        /** Aggregates all of the imported symbols for a module name. */
        class ModuleImports {
            private:
                std::vector<std::string> moduleName;
                bool rawSharedLib;
                ImportedDefVec imports;

            public:
                ModuleImports(const std::vector<std::string> &moduleName,
                              bool rawSharedLib
                              ) :
                    moduleName(moduleName),
                    rawSharedLib(rawSharedLib) {
                }

                ModuleImports(const std::vector<std::string> &moduleName,
                              bool rawSharedLib,
                              const ImportedDef &def
                              ) :
                    moduleName(moduleName),
                    rawSharedLib(rawSharedLib) {
                    imports.push_back(def);
                }

                ModuleImports() : rawSharedLib(false) {}

                void addImport(const ImportedDef &import) {
                    imports.push_back(import);
                }

                const std::vector<std::string> &getModuleName() const {
                    return moduleName;
                }

                const ImportedDefVec &getImports() const {
                    return imports;
                }

                bool isRawSharedLib() const {
                    return rawSharedLib;
                }

                bool exists() {
                    return !imports.empty();
                }
        };

    private:

        typedef std::pair<std::string, bool> ModuleId;
        typedef std::map<ModuleId, ModuleImports *> ImportsByModule;
        typedef std::map<std::string, ModuleImports *> ImportsMap;
        ImportsByModule byModule;
        ImportsMap byLocalName;

        void addImportTo(ModuleImports &imports, const ImportedDef &def);

        static ImportedDef deserializeImportSymbol(Deserializer &deser);
    public:

        /** Gets set to true if the lazy imports may be used by generics. */
        bool usedByGenerics = false;

        /**
         * Returns the ModuleImports object for the module.
         *
         * The result is mutable, you can add imports to it.
         */
        ModuleImports &getModuleImports(const std::string &moduleName,
                                        bool rawSharedLib
                                        );

        void addImport(const std::string &moduleName,
                       bool rawSharedLib,
                       const ImportedDef &def
                       );

        ~LazyImports();

        /**
         * Returns true if the lazy imports should be serialized with the
         * module meta data. (i.e. if they are non-empty and potentially used
         * by generics).
         */
        bool shouldSerialize() { return !byModule.empty() && usedByGenerics; }

        /**
         * Returns a ModuleImports object for the given symbol (and none of the
         * other symbols for the same module).
         *
         * Note that the local import may not exist, in which case this
         * returns a ModuleImports for which exists() is false and all other
         * accessors return empty.
         */
        ModuleImports getImport(const std::string &localName);

        /** Serialize the object as a sequence of LazyImportEntry fields. */
        void serialize(Serializer &serializer);

        /** Deserialize a single ModuleImports instance. */
        void deserializeModuleImports(Deserializer &deser);
};

};

#endif
