// Copyright 2010-2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ModuleDef_h_
#define _model_ModuleDef_h_

#include <set>
#include <vector>
#include "util/SourceDigest.h"
#include "LazyImports.h"
#include "ModuleDefMap.h"
#include "Namespace.h"
#include "VarDef.h"

namespace builder {
    class Builder;
}

namespace crack { namespace util {
class SourceDigest;
}}

namespace model {

SPUG_RCPTR(Context);
class Deserializer;
class GenericModuleInfo;
class Serializer;

SPUG_RCPTR(ModuleDef);

/**
 * A module.
 * The context of a module is the parent module.
 */
class ModuleDef : public VarDef, public Namespace {
    public:
        typedef std::vector<ModuleDefPtr> Vec;

    private:
        // The master module, or null if the module is its own master.  See
        // getMaster() below for info on mastership.
        // This isn't an RCPtr: masters should maintain ownership of all
        // modules in the group.
        ModuleDef *master;

        // Slave modules.  These are all of the modules that we are the
        // "master" of (see getMaster()).
        Vec slaves;

        // Compile time dependencies.
        ModuleDefMap compileTimeDeps;

        // Set of lazy imports associated with the module.
        LazyImportsPtr lazyImports;

        void serializeAliases(model::Serializer &serializer,
                              bool privateAliases,
                              const char *optionalBlockName,
                              const char *aliasTreeName
                              );

    protected:

        // Overrides Namespace::getNestedTypeDefs() to add types for all slave
        // modules.
        virtual void getNestedTypeDefs(std::vector<TypeDef*> &typeDefs,
                                       ModuleDef *master
                                       );

        // Serialize the module as a compile-time dependency.
        void serializeAsCTDep(Serializer &serializer) const;

        // Deserialize a compile-time dependency.
        // Returns a pair of the header digest (in hex) of the dependency
        // module and the canonical name of the module for purposes of error
        // reporting.  The module itself is not needed during evaluation of a
        // compile-time dependency.
        static std::pair<std::string, std::string>
            deserializeCTDep(Deserializer &deser);
    public:
        typedef std::vector<std::string> StringVec;

        // the parent namespace.  This should be the root namespace where
        // builtins are stored.
        NamespacePtr parent;

        // this is true if the module has been completely parsed and the
        // close() method has been called.
        bool finished;

        // true if this module was generated from an extension (as opposed
        // to crack source)
        bool fromExtension;

        // aliased symbols that other modules are allowed to import.
        std::map<std::string, bool> exports;

        // explicit imports.
        Vec imports;

        // Modules that we have a dependency on.
        ModuleDefMap dependencies;

        // path to original source code on disk
        std::string sourcePath;

        // MD5 digests of the source file the module was built from, the
        // meta-data and the module cache-file header.
        crack::util::SourceDigest sourceDigest, metaDigest, headerDigest;

        // true if the module should be persisted in the cache when closed.
        bool cacheable;

        // If the module is a generic instantiation, this is the path to the
        // generic type definition within the module that defines it.
        std::vector<std::string> genericName;

        // If the module is a generic instantiation, these are the parameters
        // of the generic.
        std::vector<TypeDefPtr> genericParams;

        // If the module is a generic instantiation, this is the canonical
        // name of the module.
        std::string genericModule;

        ModuleDef(const std::string &name, Namespace *parent);
        ~ModuleDef();

        /**
         * Close the module, executing it.
         */
        void close(Context &context);

        /**
         * Call the module destructor - cleans up all global variables.
         */
        virtual void callDestructor() = 0;

        virtual bool hasInstSlot() const;

        /**
         * Set namespace owner, and set our namespace name
         */
        virtual void setOwner(Namespace *o) {
            owner = o;
            canonicalName = o->getNamespaceName()+"."+name;
            fullName.clear();
        }

        /**
         * Returns the module's master, returns the module itself if it is its
         * own master.
         *
         * The "master" is the top-level module of a group of modules with
         * cyclic dependencies.  We track this relationship because it is much
         * easier to persist groups of modules with cyclic dependencies as if
         * they were a single module.
         */
        ModuleDefPtr getMaster() {
            return master ? master : this;
        }

        /**
         * Returns true if the module is a slave.
         */
        bool isSlave() { return master; }

        /**
         * Returns a vector of slave modules.
         */
        const Vec &getSlaves() const { return slaves; }

        /**
         * Record a dependency on another module.  See
         * model::Context::recordDependency() for more info.
         * Derived classes should override if it's important.
         */
        virtual void recordDependency(ModuleDef *other) {}

        /**
         * Add the other module to this module's dependencies.
         */
        void addDependency(ModuleDef *other);

        /**
         * Add a compile time dependency on the other module.  Compile time
         * dependencies are used for annotations.  They are different from
         * normal dependencies.  With normal dependencies, we only need to
         * rebuild if the dependency's interface changes.  With compile time
         * dependencies, we must rebuild if anything about the module changes.
         */
        void addCompileTimeDependency(ModuleDef *other);

        /**
         * Copies compile time dependencies from another module.  This is used
         * for generic instantiation modules.
         */
        void copyCompileTimeDepsFrom(ModuleDef *other) {
            compileTimeDeps = other->compileTimeDeps;
        }

        /**
         * Initialize compile-time dependencies for the module by calling the
         * dependency module's "main" function.
         */
        void initializeCompileTimeDeps(builder::Builder &builder) const;

        /**
         * Adds the other module to this module's slaves.
         */
        void addSlave(ModuleDef *slave);

        virtual VarDef *asVarDef();
        virtual NamespacePtr getParent(unsigned index);
        virtual NamespacePtr getNamespaceOwner();
        virtual ModuleDefPtr getModule();
        virtual bool isHiddenScope();

        void addLazyImport(const std::string &moduleName,
                           bool rawSharedLib,
                           const ImportedDef &import);

        LazyImports::ModuleImports getLazyImport(const std::string &name);

        LazyImportsPtr getLazyImports() const { return lazyImports; }
        void setLazyImports(LazyImports *imports) { lazyImports = imports; }

        /**
         * Parse a canonical module name, return it as a vector of name
         * components.
         */
        static StringVec parseCanonicalName(const std::string &name);

        /**
         * Joins name components by periods (the opposite of
         * parseCanonicalName()).
         */
        static std::string joinName(const StringVec &parts);

        /**
         * Get the "definition hash."  This is the hash of all definitions
         * exported by the module.  It is used to determine whether a
         * dependent module needs to be recompiled.
         */
        int getDefHash() const { return 0; }

        /**
         * Write the module meta-data to the serializer.
         */
        void serialize(Serializer &serializer);

        void serializeHeader(Serializer &serializer) const;

        /**
         * Deserialize the remainder of the module meta-data.
         */
        static ModuleDefPtr deserialize(Deserializer &deserializer,
                                        const std::string &canonicalName,
                                        GenericModuleInfo *genModInfo = 0
                                        );

        /**
         * Serialize the module as a slave reference.
         */
        void serializeSlaveRef(Serializer &serializer);

        /**
         * Deserialize a slave reference.
         */
        ModuleDefPtr deserializeSlaveRef(Deserializer &deser);

        /**
         * Looks up a type in the module for purposes of generic
         * instantiation.  Unlike lookUp(), this does the right thing for stub
         * modules.
         */
        virtual TypeDefPtr getType(const std::string &name);

        /**
         * Run the module main function.  This should generally only be called
         * on the top-level script, all imported modules will have their main
         * function called from that.
         */
        virtual void runMain(builder::Builder &builder) = 0;
};

} // namespace model

#endif
