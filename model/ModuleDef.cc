// Copyright 2010-2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ModuleDef.h"

#include <sstream>
#include "spug/check.h"
#include "spug/stlutil.h"
#include "spug/StringFmt.h"
#include "builder/Builder.h"
#include "util/SourceDigest.h"
#include "NamespaceAliasTreeNode.h"
#include "Context.h"
#include "Deserializer.h"
#include "Generic.h"
#include "GenericModuleInfo.h"
#include "NestedDeserializer.h"
#include "OverloadDef.h"
#include "ProtoBuf.h"
#include "Serializer.h"
#include "StatState.h"

using namespace std;
using namespace model;
using namespace crack::util;

// Serialize all of the aliases in the module.  If 'privateAliases' is
// true, just serialize the private aliases.  Otherwise just serialize the
// public aliases.
void ModuleDef::serializeOptional(Serializer &serializer,
                                  bool privateAliases,
                                  const char *optionalBlockName,
                                  const char *aliasTreeName
                                  ) {
    NamespaceAliasTreeNodePtr aliasTree = getAliasTree(privateAliases);

    // Add all of the slaves.
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves) {
        AliasTreeNodePtr slaveTree = (*slave)->getAliasTree(privateAliases);
        if (slaveTree) {
            if (!aliasTree)
                aliasTree = new NamespaceAliasTreeNode(this);

            aliasTree->addChild(slaveTree.get());
        }
    }

    // If we have an alias tree or lazy imports, write an optional block for
    // them.
    if (aliasTree ||
        lazyImports && lazyImports->shouldSerialize() && privateAliases
        ) {
        ostringstream temp;
        Serializer sub(serializer, temp);

        ostringstream temp2;
        Serializer sub2(sub, temp2);

        if (aliasTree) {
            aliasTree->serialize(sub2);
            sub.write(CRACK_PB_KEY(1, string),
                    (string(aliasTreeName) + ".header").c_str());
            sub.write(temp2.str(), aliasTreeName);
        }

        if (privateAliases && lazyImports)
            lazyImports->serialize(sub);

        serializer.write(temp.str(), optionalBlockName);
    } else {
        // No optional data.
        serializer.write(0, optionalBlockName);
    }
}

void ModuleDef::getNestedTypeDefs(std::vector<TypeDef*> &typeDefs,
                                  ModuleDef *master
                                  ) {
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        (*slave)->getTypeDefs(typeDefs, master);
}

void ModuleDef::serializeAsCTDep(Serializer &serializer) const {

    serializer.write(CRACK_PB_KEY(1, string), "canonicalName.header");
    serializer.write(getFullName(), "canonicalName");
    serializer.write(CRACK_PB_KEY(2, string), "headerDigest.header");
    serializer.write(headerDigest.asHex(), "headerDigest");
}

pair<string, string> ModuleDef::deserializeCTDep(Deserializer &deser) {
    pair<string, string> result;
    CRACK_PB_BEGIN(deser, 256, compileDep)
        CRACK_PB_FIELD(1, string) {
            result.second = compileDepDeser.readString(64, "canonicalName");
            break;
        }
        CRACK_PB_FIELD(2, string) {
            result.first = compileDepDeser.readString(32, "headerDigest");
            break;
        }
    CRACK_PB_END

    return result;
}

ModuleDef::ModuleDef(const std::string &name, Namespace *parent) :
    VarDef(0, name),
    Namespace(name),
    parent(parent),
    master(0),
    finished(false),
    fromExtension(false),
    cacheable(false) {
}

ModuleDef::~ModuleDef() {
    // We're lazily assuming that the master won't be destroyed until all of
    // his slaves are, and this currently seems to be the case.  If it ceases
    // to be the case we need to take remedial steps to ensure that we don't
    // try to serialize the slaves without their master.
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        SPUG_CHECK((*slave)->refcnt() == 1,
                   "Slave module " << (*slave)->getNamespaceName() <<
                    "would live on after deletion of its master."
                   );
}

bool ModuleDef::hasInstSlot() const {
    return false;
}

void ModuleDef::addDependency(ModuleDef *other) {
    if (other != this &&
        dependencies.find(other->getNamespaceName()) == dependencies.end()
        )
        dependencies[other->getNamespaceName()] = other;
}

void ModuleDef::addCompileTimeDependency(ModuleDef *other) {
    if (compileTimeDeps.find(other->getNamespaceName()) ==
        compileTimeDeps.end()
        )
        compileTimeDeps[other->getNamespaceName()] = other;
}

void ModuleDef::initializeCompileTimeDeps(builder::Builder &builder) const {
    SPUG_FOR(ModuleDefMap, elem, compileTimeDeps)
        elem->second->runMain(builder);
}

void ModuleDef::addSlave(ModuleDef *slave) {
    SPUG_CHECK(!slave->master,
               "Module " << slave->getNamespaceName() <<
               " is being added as a slave of " << getNamespaceName() <<
               " but it already has a master: " <<
               slave->master->getNamespaceName());
    slaves.push_back(slave);
    slave->master = this;
    slave->cacheable = false;
}

void ModuleDef::close(Context &context) {
    StatState sState(&context, ConstructStats::builder, this);
    context.builder.closeModule(context, this);
}

VarDef *ModuleDef::asVarDef() {
    return this;
}

NamespacePtr ModuleDef::getParent(unsigned index) {
    return index ? NamespacePtr(0) : parent;
}

NamespacePtr ModuleDef::getNamespaceOwner() {
    return owner;
}

ModuleDefPtr ModuleDef::getModule() {
    return this;
}

bool ModuleDef::isHiddenScope() {
    return false;
}

void ModuleDef::addLazyImport(const std::string &moduleName,
                              bool rawSharedLib,
                              const ImportedDef &import
                              ) {
    if (!lazyImports)
        lazyImports = new LazyImports();
    lazyImports->addImport(moduleName, rawSharedLib, import);
}

LazyImports::ModuleImports ModuleDef::getLazyImport(
    const std::string &localName
) {
    return lazyImports ? lazyImports->getImport(localName) :
                         LazyImports::ModuleImports();
}

LazyImportsPtr ModuleDef::getLazyImports(bool create) {
    if (!lazyImports && create)
        lazyImports = new LazyImports();
    return lazyImports;
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

string ModuleDef::joinName(const ModuleDef::StringVec &parts) {
    ostringstream result;

    bool first;
    for (StringVec::const_iterator i = parts.begin(); i < parts.end(); ++i) {
        if (!first) {
            result << '.';
            first = true;
        }
        result << *i;
    }

    return result.str();
}

#define CRACK_METADATA_V1 471296819

void ModuleDef::serialize(Serializer &serializer) {
    int id = serializer.registerObject(this);
    SPUG_CHECK(id == 0,
               "Module id for serialized module " << getFullName() <<
                " is not 0: " << id
               );
    serializer.module = this;

    serializer.digestEnabled = true;
    serializer.write(CRACK_METADATA_V1, "magic");

    // If we are a slave, just serialize a reference to the master.
    ModuleDefPtr master = getMaster();
    if (master.get() != this) {
        serializer.write(master->getFullName(), "master");
        serializer.write(0, "optional");
        return;
    } else {
        serializer.write("", "master");
    }

    // write source path and source digest
    serializer.write(sourcePath, "sourcePath");
    serializer.write(sourceDigest.asHex(), "sourceDigest");

    // write the dependencies
    serializer.write(dependencies.size(), "#deps");
    for (ModuleDefMap::const_iterator iter = dependencies.begin();
         iter != dependencies.end();
         ++iter
         ) {
        SPUG_CHECK(iter->second->master != this,
                   "module " << getFullName() << " has a dependency on "
                   "slave module " << iter->first
                   );
        serializer.write(iter->first, "canonicalName");
        serializer.write(iter->second->metaDigest.asHex(), "metaDigest");
        serializer.write(0, "optional");
    }

    if (compileTimeDeps.size() || genericName.size() || genericParams.size()) {
        ostringstream temp;
        Serializer sub(serializer, temp);
        SPUG_FOR(ModuleDefMap, iter, compileTimeDeps) {
            ostringstream depOut;
            Serializer depSer(sub, depOut);
            iter->second->serializeAsCTDep(depSer);
            sub.write(CRACK_PB_KEY(1, string), "compileDep.header");
            sub.write(depOut.str(), "compileDep");
        }

        SPUG_FOR(vector<string>, iter, genericName) {
            sub.write(CRACK_PB_KEY(2, string), "genericName.header");
            sub.write(*iter, "genericName");
        }

        SPUG_FOR(vector<TypeDefPtr>, iter, genericParams) {
            sub.write(CRACK_PB_KEY(3, ref), "genericParams.header");
            (*iter)->serializeExternRef(sub, 0);
        }

        if (genericModule.size()) {
            sub.write(CRACK_PB_KEY(4, string), "genericModule.header");
            sub.write(genericModule, "genericModule");
        }

        serializer.write(temp.str(), "optional");
    } else {
        serializer.write(0, "optional");
    }
    serializer.digestEnabled = false;
    headerDigest = serializer.hasher.getDigest();
    serializer.hasher.reset();

    // end of Header

    // write all of the symbols
    serializer.digestEnabled = true;
    Namespace::serializeTypeDecls(serializer, this);

    // Build an array of all public types and their private dependencies
    // in order of dependencies before dependents.
    Namespace::OrderedTypes types;
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        (*slave)->getOrderedTypes(types, this);
    getOrderedTypes(types, this);

    // Now serialize this type array.
    serializer.write(types.size(), "#types");
    SPUG_FOR(Namespace::OrderedTypes, iter, types)
        (*iter)->serializeDef(serializer);

    vector<const Namespace *> allNamespaces;
    allNamespaces.push_back(this);
    SPUG_FOR(vector<ModuleDefPtr>, slave, slaves)
        allNamespaces.push_back(slave->get());
    Namespace::serializeNonTypeDefs(allNamespaces, serializer);

    // write all of the exports
    serializer.write(exports.size(), "#exports");
    for (std::map<std::string, bool>::iterator iter = exports.begin();
         iter != exports.end();
         ++iter
         )
        serializer.write(iter->first, "exports");

    // Write all of the public aliases.
    serializeOptional(serializer, false, "optional", "aliasTree");

    // sign the metadata
    serializer.digestEnabled = false;
    metaDigest = serializer.hasher.getDigest();

    // Now write private aliases.
    serializeOptional(serializer, true, "optionalPostDigest",
                      "privateAliasTree"
                      );
}

void ModuleDef::serializeHeader(Serializer &serializer) const {
    if (Serializer::trace)
        cerr << "# kind = module" << endl;
    serializer.write(Serializer::slaveModuleId, "kind");
    serializer.write(canonicalName, "name");
}

namespace {
    void deserializeOptional(Deserializer &deser, ModuleDef *mod,
                             const char *aliasTreeName) {
        // Read optional data.
        CRACK_PB_BEGIN(deser, 256, optional)
            CRACK_PB_FIELD(1, string) {
                string temp = optionalDeser.readString(256, aliasTreeName);
                istringstream tempSrc(temp);
                Deserializer tempDeser(optionalDeser, tempSrc);

                // Aliases.
                SPUG_CHECK(
                    tempDeser.readUInt("kind") ==
                     Serializer::slaveModuleId,
                    "Bad 'kind' parameter in cached module alias section!"
                );
                SPUG_CHECK(
                    tempDeser.readString(Serializer::varNameSize, "name") ==
                     mod->getNamespaceName(),
                    "Bad 'name' parameter in cached module alias section!"
                );
                mod->deserializeAliases(tempDeser);
                break;
            }

            CRACK_PB_FIELD(2, string) {
                LazyImportsPtr lazyImports = mod->getLazyImports(true);
                lazyImports->deserializeModuleImports(optionalDeser);
                break;
            }
        CRACK_PB_END
    }
}

ModuleDefPtr ModuleDef::deserialize(Deserializer &deser,
                                    const string &canonicalName,
                                    GenericModuleInfo *genModInfo
                                    ) {
    if (Serializer::trace)
        cerr << ">>>> Deserializing module " << canonicalName << endl;

    deser.digestEnabled = true;
    if (deser.readUInt("magic") != CRACK_METADATA_V1)
        return 0;

    string master = deser.readString(Serializer::modNameSize, "master");
    if (master.size()) {
        // Make sure we have the master.   In theory, we cannot reference a
        // slave without having referenced the master.
        Construct &construct = *deser.context->construct;
        construct.getModule(master);

        // Optional fields for Header, reading it now prior to leaving this
        // function.
        deser.readString(64, "optional");

        // Now we should be able to load the slave, or it doesn't exist as a
        // slave any more.
        Construct::ModuleMap::iterator iter =
            construct.moduleCache.find(canonicalName);
        if (Serializer::trace)
            cerr << ">>>> Finished deserializing slave " << canonicalName <<
                endl;
        if (iter != construct.moduleCache.end())
            return iter->second;
        else
            return 0;
    }

    string sourcePath = deser.readString(Serializer::modNameSize, "sourcePath");
    SourceDigest recordedSourceDigest =
        SourceDigest::fromHex(deser.readString(Serializer::modNameSize,
                                               "sourceDigest"
                                               )
                              );

    // We're going to complete the serialization at this point so we can read
    // the generic name and params if they exist, so we keep track of whether
    // we have to rebuild anyway.
    bool mustRebuild = false;

    // check the digest against that of the actual source file (if the source
    // file can be found)
    if (sourcePath.size()) {
        Construct::ModulePath modPath =
            deser.context->construct->searchSourcePath(sourcePath);
        if (modPath.found) {
            SourceDigest fileDigest = SourceDigest::fromFile(modPath.path);
            if (fileDigest != recordedSourceDigest) {
                if (Construct::traceCaching)
                    cerr << "digests don't match for " << sourcePath <<
                        " got " << recordedSourceDigest.asHex() <<
                        "\n  current = " <<
                        fileDigest.asHex() << "\n  module: " <<
                        canonicalName << endl;
                mustRebuild = true;
            }
        } else if (!deser.context->construct->allowSourceless) {
            deser.context->error(SPUG_FSTR("No source file found for cached "
                                            "module " << canonicalName << ".  Use "
                                            "--allow-sourceless to allow loading "
                                            "from the cache."
                                        )
                                );
        }
    } else if (Construct::traceCaching) {
        cerr << "Not checking source digest for sourceless module " <<
            canonicalName << endl;
    }

    // See if the builder can open its file.
    builder::Builder::CacheFilePtr builderCache;
    if (!mustRebuild) {
        builderCache =
            deser.context->builder.getCacheFile(*deser.context, canonicalName);
        if (!builderCache) {
            if (Construct::traceCaching)
                cerr << "No builder cache file for " << sourcePath << "@" <<
                    recordedSourceDigest.asHex() << endl;
            mustRebuild = true;
        }
    }

    // read and load the dependencies
    int count = deser.readUInt("#deps");
    for (int i = 0; i < count; ++i) {
        string depName = deser.readString(64, "canonicalName");
        SourceDigest moduleDigest =
            SourceDigest::fromHex(deser.readString(64, "metaDigest"));
        deser.readString(64, "optional");

        if (!mustRebuild) {
            ModuleDefPtr mod = deser.context->construct->getModule(depName);

            // If we didn't get the module, we need to rebuild.
            if (!mod) {
                mustRebuild = true;
                continue;

            // if the dependency isn't finished, don't do a depdendency check.
            // Note: I don't know why we hit this, but we seem to.
            } else if (!mod->finished) {
                continue;
            }

            // if the dependency has a different definition hash from what we were
            // built against, we have to recompile.
            if (mod->metaDigest != moduleDigest) {
                if (Construct::traceCaching)
                    cerr << "meta digest doesn't match for dependency " <<
                        mod->getFullName() << ", need to rebuild " <<
                        canonicalName << "(depending on " <<
                        moduleDigest.asHex() <<
                        " current = " << mod->metaDigest.asHex() << ")" << endl;
                mustRebuild = true;
            }
        }
    }

    vector<string> genericName;
    vector<TypeDefPtr> genericParams;
    string genericModule;
    ModuleDefMap compileTimeDeps;

    CRACK_PB_BEGIN(deser, 256, optional)
        CRACK_PB_FIELD(1, string) {
            pair<string, string> depInfo = deserializeCTDep(optionalDeser);
            if (mustRebuild)
                break;

            ModuleDefPtr depMod =
                deser.context->construct->getModule(depInfo.second);
            compileTimeDeps[depInfo.second] = depMod.get();
            bool upToDate = depMod && depMod->finished &&
                            depMod->headerDigest.asHex() == depInfo.first;
            if (!upToDate) {
                if (Construct::traceCaching) {
                    if (!depMod)
                        cerr << "No cache module for compile-time "
                            "dependency " << depMod->getFullName() <<
                            ", need to rebuild " << canonicalName << endl;
                    else
                        cerr << "header digest doesn't match for "
                            "compile-time dependency on " <<
                            depMod->getFullName() <<
                            ", need to rebuild " << canonicalName <<
                            " (depending on " << depInfo.first << ", got " <<
                            depMod->headerDigest.asHex() <<  ")" <<
                            endl;
                }
                mustRebuild = true;
            }
            break;
        }
        CRACK_PB_FIELD(2, string) {
            genericName.push_back(
                optionalDeser.readString(64, "genericName")
            );
            break;
        }
        CRACK_PB_FIELD(3, ref) {
            genericParams.push_back(
                TypeDefPtr::rcast(TypeDef::deserializeRef(optionalDeser, "ext"))
            );
            break;
        }
        CRACK_PB_FIELD(4, string) {
            genericModule = optionalDeser.readString(64, "genericModule");
            break;
        }
    CRACK_PB_END

    if (genModInfo && genericName.size()) {
        genModInfo->present = true;
        genModInfo->name = genericName;
        genModInfo->params = genericParams;
        genModInfo->module = genericModule;
    }

    // The cached meta-data is up-to-date.
    deser.digestEnabled = false;

    // deserialize the actual code through the builder.
    ModuleDefPtr mod;
    if (!mustRebuild) {
        mod = deser.context->builder.materializeModule(*deser.context,
                                                       builderCache.get(),
                                                       canonicalName,
                                                       0 // owner
                                                       );
    }

    if (!mod) {
        if (Serializer::trace)
            cerr << ">>>> Finished deserializing header, must rebuild." <<
                canonicalName << endl;
        return 0;
    }

    // Set the header digest from what we discovered.
    mod->headerDigest = deser.hasher.getDigest();
    deser.hasher.reset();

    // Set the generic name and params.
    mod->genericName = genericName;
    mod->genericParams = genericParams;
    mod->genericModule = genericModule;

    mod->compileTimeDeps = compileTimeDeps;

    // storing the module in the construct cache - this is actually also done
    // later within construct, but we need the module to be present while
    // we're constructing it so we can resolve types by name when building
    // them.
    deser.context->construct->moduleCache[canonicalName] = mod;

    // register the module as id 0.
    deser.registerObject(0, mod.get());

    deser.context->ns = mod.get();
    deser.digestEnabled = true;
    mod->deserializeTypeDecls(deser);

    // Deserialize all of the types.
    count = deser.readUInt("#types");
    for (int i = 0; i < count; ++i)
        TypeDef::deserializeTypeDef(deser, "type");

    mod->deserializeDefs(deser);

    // deserialize exports
    int exportsCount = deser.readUInt("#exports");
    for (int i = 0; i < exportsCount; ++i)
        mod->exports[deser.readString(Serializer::varNameSize, "exports")] =
            true;

    // Deserialize the public aliases.
    deserializeOptional(deser, mod.get(), "aliasTree");

    mod->metaDigest = deser.hasher.getDigest();
    mod->sourcePath = sourcePath;
    mod->sourceDigest = recordedSourceDigest;

    // Now do the private aliases.
    deserializeOptional(deser, mod.get(), "privateAliasTree");

    // If we ended up with lazy imports, propagate those to all generics.
    if (mod->lazyImports) {
        SPUG_FOR(vector<TypeDefPtr>, iter, deser.getRegisteredGenerics())
            (*iter)->genericInfo->lazyImports = mod->lazyImports;
    }

    if (Serializer::trace)
        cerr << ">>>> Finished deserializing module " << canonicalName << endl;
    return mod;
}

void ModuleDef::serializeSlaveRef(Serializer &serializer) {
    if (serializer.writeObject(this, "owner")) {
        serializer.write(canonicalName, "canonicalName");
        serializer.write(0, "optional");
    }
}

namespace {
    struct SlaveModuleReader : public Deserializer::ObjectReader {
        ModuleDefPtr master;
        SlaveModuleReader(ModuleDef *master) : master(master) {}
        virtual spug::RCBasePtr read(Deserializer &deser) const {
            string name = deser.readString(Serializer::modNameSize,
                                           "canonicalName"
                                           );
            deser.readString(64, "optional");
            ModuleDefPtr mod = deser.context->builder.materializeModule(
                *deser.context,
                deser.context->builder.getCacheFile(*deser.context,
                                                    name
                                                    ).get(),
                name,
                master.get()
            );
            deser.context->construct->moduleCache[name] = mod;
            return mod;
        }
    };
}

ModuleDefPtr ModuleDef::deserializeSlaveRef(Deserializer &deser) {
    Deserializer::ReadObjectResult readObj =
        deser.readObject(SlaveModuleReader(this), "owner");
    return ModuleDefPtr::arcast(readObj.object);
}

TypeDefPtr ModuleDef::getType(const string &name) {
    return lookUp(name);
}
