// Copyright 2009 Google Inc.

#ifndef _model_Namespace_h_
#define _model_Namespace_h_

#include <map>
#include <vector>
#include <spug/RCBase.h>
#include <spug/RCPtr.h>

namespace model {

class Context;
SPUG_RCPTR(Expr);
SPUG_RCPTR(FuncDef);
SPUG_RCPTR(OverloadDef);
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

    protected:        
        VarDefMap defs;

        // in an "instance" context, this maintains the order of declaration 
        // of the instance variables so we can create and delete in the 
        // correct order.
        VarDefVec ordered;

        // fully qualified namespace name, e.g. "crack.io"
        std::string canonicalName;

        /**
         * Stores a definition, promoting it to an overload if necessary.
         */
        virtual void storeDef(VarDef *def);

    public:
        
        Namespace(const std::string &cName) :
                canonicalName(cName)
        {

        }

        /**
         * Returns the fully qualified name of the namespace
         */
        const std::string &getNamespaceName() const { return canonicalName; }

        /** 
         * Returns the parent at the index, null if it is greater than 
         * the number of parents.
         */
        virtual NamespacePtr getParent(unsigned index) = 0;

        VarDefPtr lookUp(const std::string &varName, bool recurse = true);

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
        virtual void addAlias(const std::string &name, VarDef *def);
        
        /** Alias all symbols from the other namespace. */
        void aliasAll(Namespace *other);
        
        /**
         * Replace an existing defintion with the new definition.
         * This is only used to replace a StubDef with an external function 
         * definition.
         */
        virtual void replaceDef(VarDef *def);

        void dump(std::ostream &out, const std::string &prefix);
        void dump();
        
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
};

} // namespace model

#endif
