// Copyright 2010,2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _model_Namespace_h_
#define _model_Namespace_h_

#include <map>
#include <set>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
class Deserializer;
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
SPUG_RCPTR(ModuleDef);
SPUG_RCPTR(OverloadDef);
class Serializer;
class TypeDef;
SPUG_RCPTR(VarDef);

SPUG_RCPTR(Namespace);

/**
 * A namespace is a symbol table.  It defines the symbols that exist in a 
 * given context.
 */
class Namespace : public virtual spug::RCBase {

    public:
        typedef std::map<std::string, VarDefPtr> VarDefMap;
        typedef std::vector<VarDefPtr> VarDefVec;
        
        /**
         * A collection of types that contains both a vector (to maintain 
         * order of addition) and a set (to allow us to efficiently check if a 
         * type is in the collection).
         */
        class OrderedTypes {
            private:
                std::vector<const TypeDef *> ordered;
                std::set<const TypeDef *> indexed;
                
            public:
                typedef std::vector<const TypeDef *>::const_iterator const_iterator;

                /**
                 * Adds 'type' and all of its dependencies to the collection 
                 * (depdendencies first), excluding types that are already 
                 * present in the collection and those that are not 
                 * copersistent with the given master module.
                 */
                void add(const TypeDef *type, const ModuleDef *master);
                
                /** Returns true if the collection contains 'type'. */
                bool contains(const TypeDef *type) {
                    return indexed.find(type) != indexed.end();
                }
                
                const_iterator begin() const { return ordered.begin(); }
                const_iterator end() const { return ordered.end(); } 
                size_t size() const { return ordered.size(); }
        };

    protected:        
        VarDefMap defs;

        // in an "instance" context, this maintains the order of declaration 
        // of the instance variables so we can create and delete in the 
        // correct order.
        VarDefVec ordered;

        // ordered list of _all_ vardefs. XXX this is only needed for
        // caching currently
        VarDefVec orderedForCache;

        // fully qualified namespace name, e.g. "crack.io"
        std::string canonicalName;

        /**
         * Stores a definition, promoting it to an overload if necessary.
         */
        virtual void storeDef(VarDef *def);

        /**
         * Collect all of the type definitions in the namespace into 
         * 'typeDefs'.  Note that 'typeDefs' is an array of raw pointers, the 
         * definitions must not be mutated while the vector is in existence, 
         * otherwise the type defs could be garbage collected and the array 
         * could be left with dangling pointers.
         * @param master if provided this is the master module to serialize 
         *     under.
         */
        void getTypeDefs(std::vector<TypeDef*> &typeDefs,
                         ModuleDef *master
                         );
        
        /**
         * Add type definitions of nested child namespaces to 'typeDefs'.  By 
         * default, does nothing.  Derived classes that can contain publically 
         * visible types should override.
         */
        virtual void getNestedTypeDefs(std::vector<TypeDef*> &typeDefs,
                                       ModuleDef *master
                                       ) {
        }

        void deserializeDefs(Deserializer &deser, const char *countName,
                             bool publicDefs
                             );

    public:
        
        Namespace(const std::string &cName) : canonicalName(cName) {}
        
        /**
         * Returns the fully qualified name of the namespace
         */
        const std::string &getNamespaceName() const { return canonicalName; }
        
        void setNamespaceName(const std::string &name) {
            canonicalName = name;
        }
        
        /**
         * Converts the Namespace to a VarDef, if it is one.  Otherwise, 
         * returns null.
         */
        virtual VarDef *asVarDef();

        /** 
         * Returns the parent at the index, null if it is greater than 
         * the number of parents.
         */
        virtual NamespacePtr getParent(unsigned index) = 0;
        
        /**
         * Returns the owner of the namespace.
         */
        virtual NamespacePtr getNamespaceOwner();

        VarDefPtr lookUp(const std::string &varName, bool recurse = true);
        
        /**
         * Returns the module that the namespace is part of.
         */
        virtual ModuleDefPtr getModule() = 0;
        
        /**
         * Returns true if symbols in the namespace cannot be accessed by 
         * other modules.
         */
        virtual bool isHiddenScope();

        /**
         * Returns true if the container directly or indirectly contains 
         * generics.  Note that this only considers the immediate namespace, 
         * not parent namespaces.
         */
        virtual bool hasGenerics() const;

        /**
         * Returns the "real module" that the namespace is part of.  If the 
         * namespace is part of an ephemeral module generated for a generic, 
         * the real module is the module that the generic was defined in.
         * This is equivalent to the owner of the module returned by 
         * getModule() if there is one, and simply the module returned by 
         * getModule() if it doesn't have an owner.
         */
        ModuleDefPtr getRealModule();

        /**
         * Returns true if the definition is aliased in the namespace.
         */
        bool hasAliasFor(VarDef *def) const;

        /**
         * Returns true if the definition is an alias in the namespace.
         * 'def' is assumed to be a definition stored in the namespace under 
         * 'name', though this function doesn't verify that it is.
         */
        bool isAlias(const VarDef *def, const std::string &name) const;

        /**
         * Add a new definition to the namespace (this may not be used for 
         * FuncDef's, these must be wrapped in an OverloadDef.  See Context 
         * for an easy way to add FuncDef's)
         */        
        virtual void addDef(VarDef *def);
        
        /** 
         * Remove a definition.  Intended for use with stubs - "def" must not 
         * be an OverloadDef. 
         */
        virtual void removeDef(VarDef *def);
        
        /**
         * Adds a definition to the context, but does not make the definition's 
         * context the context.  This is used for importing symbols into a 
         * module context.
         */
        virtual void addAlias(VarDef *def);
        virtual OverloadDefPtr addAlias(const std::string &name, 
                                        VarDef *def
                                        );

        /**
         * Adds an alias without special processing if 'def' is an overload.  
         * This is only safe in a situation where we know that the overload 
         * can never be extended in a new context, which happens to be the 
         * case when we're aliasing symbols from .builtin to the root module.
         */
        virtual void addUnsafeAlias(const std::string &name, VarDef *def);
        
        /** 
         * Alias all symbols from the other namespace and all of its ancestor 
         * namespaces. 
         */
        void aliasAll(Namespace *other);
        
        /**
         * Replace an existing defintion with the new definition.
         * This is only used to replace a StubDef with an external function 
         * definition.
         */
        virtual OverloadDefPtr replaceDef(VarDef *def);

        void dump(std::ostream &out, const std::string &prefix) const;
        void dump() const;
        
        /** Funcs to iterate over the set of definitions. */
        /// @{
        VarDefMap::iterator beginDefs() { return defs.begin(); }
        VarDefMap::iterator endDefs() { return defs.end(); }
        VarDefMap::const_iterator beginDefs() const { return defs.begin(); }
        VarDefMap::const_iterator endDefs() const { return defs.end(); }
        /// @}
        
        /** Funcs to iterate over the definitions in order of declaration. */
        /// @{
        VarDefVec::iterator beginOrderedDefs() { return ordered.begin(); }
        VarDefVec::iterator endOrderedDefs() { return ordered.end(); }
        VarDefVec::const_iterator beginOrderedDefs() const {
            return ordered.begin();
        }
        VarDefVec::const_iterator endOrderedDefs() const {
            return ordered.end();
        }
        /// @}

        /** XXX Cache ordered vector */
        /// @{
        VarDefVec::const_iterator beginOrderedForCache() const {
            return orderedForCache.begin();
        }
        VarDefVec::const_iterator endOrderedForCache() const {
            return orderedForCache.end();
        }
        /// @}

        /**
         * Add all of the types in the namespace to 'types'.
         */
        void getOrderedTypes(OrderedTypes &types, const ModuleDef *master) const;

        /**
         * Serialize all type declarations in the namespace (and nested 
         * namespaces.
         */
        void serializeTypeDecls(Serializer &serializer, ModuleDef *master);
        
        /**
         * Serialize all of the definitions in all of the namespaces.
         */
        void serializeNonTypeDefs(
            const std::vector<const Namespace *>& namespaces,
            Serializer &serializer
        ) const;

        /**
         * Serialize all of the definitions in the namespace.
         */
        void serializeNonTypeDefs(Serializer &serializer) const {
            std::vector<const Namespace*> namespaces;
            namespaces.push_back(this);
            serializeNonTypeDefs(namespaces, serializer);
        }
        
        /** 
         * Deserialize an array of definitions and store them in the 
         * namespace.
         */
        void deserializeDefs(Deserializer &deser);
        
        /**
         * Deserialize type declarations, adding them to the object id table.
         * Returns the next object id after deserialization.
         */
        static void deserializeTypeDecls(Deserializer &deser);
};

inline std::ostream &operator <<(std::ostream &out, const Namespace &ns) {
    ns.dump(out, "");
    return out;
}

} // namespace model

#endif
