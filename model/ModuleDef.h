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
#include "Namespace.h"
#include "VarDef.h"

namespace crack { namespace util {
class SourceDigest;
}}

namespace model {

SPUG_RCPTR(Context);
class Deserializer;
class Serializer;

SPUG_RCPTR(ModuleDef);

/**
 * A module.
 * The context of a module is the parent module.
 */
class ModuleDef : public VarDef, public Namespace {
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

        // path to original source code on disk
        std::string sourcePath;

        ModuleDef(const std::string &name, Namespace *parent);

        /**
         * Close the module, executing it.
         */
        void close(Context &context);
        
        /**
         * Call the module destructor - cleans up all global variables.
         */
        virtual void callDestructor() = 0;
        
        virtual bool hasInstSlot();

        /**
         * Set namespace owner, and set our namespace name
         */
        virtual void setOwner(Namespace *o) {
            owner = o;
            canonicalName = o->getNamespaceName()+"."+name;
            fullName.clear();
        }

        /**
         * Record a dependency on another module.  See 
         * model::Context::recordDependency() for more info.
         * Derived classes should override if it's important.
         */
        virtual void recordDependency(ModuleDef *other) {}

        /**        
         * Returns true if the module matches the source file found along the 
         * given path.
         */
        bool matchesSource(const StringVec &libSearchPath);
        
        /**
         * Returns true if the module matches the specific source file.
         */
        virtual bool matchesSource(const std::string &sourcePath) = 0;

        virtual NamespacePtr getParent(unsigned index);
        virtual ModuleDefPtr getModule();
        
        /**
         * Parse a canonical module name, return it as a vector of name 
         * components.
         */
        static StringVec parseCanonicalName(const std::string &name);

        /**
         * Get the "definition hash."  This is the hash of all definitions 
         * exported by the module.  It is used to determine whether a 
         * dependent module needs to be recompiled.
         */
        int getDefHash() const { return 0; }

        /**
         * Write the module meta-data to the serializer.
         */
        void serialize(Serializer &serializer) const;
        
        /**
         * Read the module header from the serializer, return true if the 
         * cache data appears to be up-to-date.
         */
        static bool readHeaderAndVerify(Deserializer &serializer,
                                        const crack::util::SourceDigest &digest
                                        );

        /**
         * Deserialize the remainder of the module meta-data.
         */        
        static ModuleDefPtr deserialize(Deserializer &deserializer);
};

} // namespace model

#endif
