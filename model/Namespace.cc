// Copyright 2010,2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 


#include "Namespace.h"

#include <sstream>

#include "spug/check.h"
#include "spug/stlutil.h"
#include "ConstVarDef.h"
#include "Context.h"
#include "Deserializer.h"
#include "Expr.h"
#include "OverloadDef.h"
#include "ProtoBuf.h"
#include "Serializer.h"
#include "StubDef.h"
#include "VarDef.h"

using namespace spug;
using namespace std;
using namespace model;

void Namespace::storeDef(VarDef *def) {
    assert(!FuncDefPtr::cast(def) && 
           "it is illegal to store a FuncDef directly (should be wrapped "
           "in an OverloadDef)");
    defs[def->name] = def;
    orderedForCache.push_back(def);
}

void Namespace::getTypeDefs(std::vector<TypeDef*> &typeDefs, 
                            ModuleDef *master
                            ) {
    for (VarDefMap::const_iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         ) {
        TypeDef *def = TypeDefPtr::rcast(iter->second);
        if (def) {
            // Ignore types we don't own or are not serializable.
            if (def->getModule()->getMaster() == master && 
                def->isSerializable()
                )
                typeDefs.push_back(def);
        } else {
            SPUG_CHECK(!NamespacePtr::rcast(iter->second), 
                       "found a non-type namespace: " << iter->first
                       );
        }
    }
    getNestedTypeDefs(typeDefs, master);
}

void Namespace::deserializeDefs(Deserializer &deser, const char *countName,
                                bool publicDefs) {
    // read all of the symbols
    unsigned count = deser.readUInt(countName);
    for (int i = 0; i < count; ++i) {
        int kind = deser.readUInt("kind");
        switch (static_cast<Serializer::DefTypes>(kind)) {
            case Serializer::variableId:
                addDef(VarDef::deserialize(deser).get());
                break;
            case Serializer::typeAliasId:
            case Serializer::aliasId: {
                string alias = 
                    deser.readString(Serializer::varNameSize, "alias");
                VarDefPtr varDef;
                if (static_cast<Serializer::DefTypes>(kind) == 
                    Serializer::typeAliasId
                    )
                    varDef = TypeDef::deserializeRef(deser);
                else
                    varDef = VarDef::deserializeAlias(deser);
                addAlias(alias, varDef.get());
                
                // if we are doing public defs in module scope, this alias 
                // has to be a second-order import.
                if (publicDefs) {
                    ModuleDef *mod = ModuleDefPtr::cast(this);
                    if (mod)
                        mod->exports[alias] = true;
                }
                break;
            }
            case Serializer::genericId:
                // XXX don't think we need this, generics are probably stored 
                // in a type.
                SPUG_CHECK(false, "can't deserialize generics yet");
//                addDef(Generic::deserialize(deser));
                break;
            case Serializer::overloadId: {
                OverloadDefPtr ovld = 
                    OverloadDef::deserialize(deser, this).get();
                addDef(ovld.get());
                addDefToMeta(ovld.get());
                break;
            }
            case Serializer::typeId:
                TypeDef::deserializeTypeDef(deser);
                break;
            case Serializer::constVarId:
                addDef(ConstVarDef::deserialize(deser).get());
                break;
            default:
                SPUG_CHECK(false, "Bad definition type id " << kind);
        }
    }
}

VarDef *Namespace::asVarDef() {
    // By default, namespaces are not VarDefs.
    return 0;
}

NamespacePtr Namespace::getNamespaceOwner() {
    return getParent(0);
}

VarDefPtr Namespace::lookUp(const std::string &varName, bool recurse) {
    VarDefMap::iterator iter = defs.find(varName);
    if (iter != defs.end()) {
        return iter->second;
    } else if (recurse) {
        VarDefPtr def;

        // try to find the definition in the parents
        NamespacePtr parent;
        for (unsigned i = 0; parent = getParent(i++);)
            if (def = parent->lookUp(varName))
                break;

        return def;        
    }

    return 0;
}

bool Namespace::isHiddenScope() {
    // all scopes are hidden by default.
    return true;
}

bool Namespace::hasGenerics() const {
    SPUG_FOR(VarDefMap, i, defs) {
        Namespace *ns = NamespacePtr::rcast(i->second);
        if (ns && ns->hasGenerics())
            return true;
    }
    return false;
}

ModuleDefPtr Namespace::getRealModule() {
    ModuleDefPtr mod = getModule();
    if (mod) {
        NamespacePtr owner = mod->getOwner();
        if (owner)
            return owner->getRealModule();
    }
    return mod;
}

bool Namespace::hasAliasFor(VarDef *def) const {
    for (VarDefMap::const_iterator iter = defs.begin(); iter != defs.end(); 
         ++iter
         )
        if (iter->second.get() == def)
            return true;
    return false;
}

void Namespace::addDef(VarDef *def) {
    assert(!def->getOwner());

    storeDef(def);
    def->setOwner(this);
}

void Namespace::addDefToMeta(OverloadDef *def) {}

void Namespace::removeDef(VarDef *def) {
    assert(!OverloadDefPtr::cast(def));
    VarDefMap::iterator iter = defs.find(def->name);
    assert(iter != defs.end());
    defs.erase(iter);

    // remove it from the ordered defs
    for (VarDefVec::iterator iter = ordered.begin();
         iter != ordered.end();
         ++iter
         ) {
        ordered.erase(iter);
        break;
    }
    
    // remove it from the ordered for cache defs
    for (VarDefVec::iterator iter = orderedForCache.begin();
         iter != orderedForCache.end();
         ++iter
         ) {
        orderedForCache.erase(iter);
        break;
    }
}

void Namespace::addAlias(VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->getOwner());

    // overloads should never be aliased - otherwise the new context could 
    // extend them.
    OverloadDef *overload = OverloadDefPtr::cast(def);
    if (overload) {
        OverloadDefPtr child = overload->createAlias();
        storeDef(child.get());
        child->setOwner(this);
    } else {
        storeDef(def);
    }
}

OverloadDefPtr Namespace::addAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->getOwner());

    // overloads should never be aliased - otherwise the new context could 
    // extend them.
    OverloadDef *overload = OverloadDefPtr::cast(def);
    if (overload) {
        OverloadDefPtr child = overload->createAlias();
        defs[name] = child.get();
        child->setOwner(this);
        return child;
    } else {
        defs[name] = def;
        return 0;
    }
}

void Namespace::addUnsafeAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    assert(def->getOwner());
    defs[name] = def;
}

void Namespace::aliasAll(Namespace *other) {
    for (VarDefMap::iterator iter = other->beginDefs();
         iter != other->endDefs();
         ++iter
         )
        if (!lookUp(iter->first))
            addAlias(iter->second.get());
    
    // do parents afterwards - since we don't clobber existing aliases, we 
    // want to do the innermost names first.
    NamespacePtr parent;
    for (int i = 0; parent = other->getParent(i++);) {
        aliasAll(parent.get());
    }
}

OverloadDefPtr Namespace::replaceDef(VarDef *def) {
    SPUG_CHECK(!def->getOwner(), 
               "Namespace::replaceDef() called on " << def->getFullName() << 
               ", which already has an owner."
               );
    StubDefPtr existing = defs[def->name];
    SPUG_CHECK(StubDefPtr::rcast(existing),
               "Namespace::replaceDef() called on " << def->getFullName() <<
               ", which is not a stub (the code currently assumes a stub)"
               );
            
    OverloadDefPtr ovld;
    if (!(ovld = OverloadDefPtr::cast(def))) {
        FuncDefPtr func = FuncDefPtr::cast(def);
        SPUG_CHECK(func, 
                   "Replacing " << def->getFullName() << 
                   " with a non function."
                   );
        ovld = new OverloadDef(def->name);
        ovld->collectAncestors(this);
        ovld->addFunc(func.get());
        func->setOwner(this);
        def = ovld.get();
    }
    def->setOwner(this);
    defs[def->name] = def;
    return ovld;
}

void Namespace::dump(ostream &out, const string &prefix) const {
    out << canonicalName << " (0x" << this << ") {\n";
    string childPfx = prefix + "  ";
    unsigned i = 0;
    Namespace *parent;
    while (parent = const_cast<Namespace *>(this)->getParent(i++).get()) {
        out << childPfx << "parent namespace ";
        parent->dump(out, childPfx);
    }
    
    for (VarDefMap::const_iterator varIter = defs.begin();
         varIter != defs.end();
         ++varIter
         )
        varIter->second->dump(out, childPfx);
    out << prefix << "}\n";
}

void Namespace::dump() const {
    dump(cerr, "");
}

void Namespace::serializeTypeDecls(Serializer &serializer) {
    // We build a vector so we can determine the count up front.
    vector<TypeDef *> typeDefs;
    getTypeDefs(typeDefs, getModule().get());
    
    serializer.write(typeDefs.size(), "#decls");
    for (vector<TypeDef *>::const_iterator iter = typeDefs.begin();
         iter != typeDefs.end();
         ++iter
         ) {
        (*iter)->serializeDecl(serializer);
    }
}

namespace {
    // Adds a type to an ordered vector, ensuring that the base class types 
    // have been serialized first.
    void addTypeToOrderedVec(const Namespace *ns,
                             vector<TypeDef *> &outputVec, 
                             TypeDef *type
                             ) {
        // ignore if we get to an unserializable type.
        if (!type->isSerializable() || type->getOwner() != ns)
            return;

        // Make sure the bases have been serialized.
        for (int i = 0; i < type->parents.size(); ++i)
            addTypeToOrderedVec(ns, outputVec, type->parents[i].get());
        
        // serialize the type itself
        outputVec.push_back(type);
    }
}

void Namespace::serializeDefs(Serializer &serializer) const {

    // If the namespace has generics, we need to serialize private definitions.
    bool serializePrivates = hasGenerics();
    
    // Count the number of definitions to serialize and separate out the 
    // types, aliases and everything else.
    int count = 0;
    set<TypeDef *> types, privateTypes;
    vector<VarDef *> others, otherPrivates;
    typedef vector< pair<string, VarDefPtr> > VarDefVec;
    VarDefVec aliases, privateAliases;
    SPUG_FOR(VarDefMap, i, defs) {
        if (i->second->isSerializable()) {
            
            // is it an alias?
            if (i->second->getOwner() != this) {
                if (i->second->isImportable(this, i->first))
                    aliases.push_back(*i);
                else if (serializePrivates)
                    privateAliases.push_back(*i);
                continue;
            }
  
            // is this a typedef?
            TypeDef *def = TypeDefPtr::rcast(i->second);
            if (def) {
                if (def->isImportable(this, i->first))
                    types.insert(def);
                else if (serializePrivates)
                    privateTypes.insert(def);
            } else {
                if (i->second->isImportable(this, i->first))
                    others.push_back(i->second.get());
                else if (serializePrivates)
                    otherPrivates.push_back(i->second.get());
            }
        }
    }

    // Put the types in order.    
    vector<TypeDef *> orderedTypes;
    SPUG_FOR(set<TypeDef *>, i, types)
        addTypeToOrderedVec(this, orderedTypes, *i);
    
    // Add the private types, keep track of where they start.
    int privateStart = orderedTypes.size();
    SPUG_FOR(set<TypeDef *>, i, privateTypes)
        addTypeToOrderedVec(this, orderedTypes, *i);
        
    // write the count and the definitions
    serializer.write(aliases.size() + privateStart + others.size(), "#defs");
    
    // first the aliases
    SPUG_FOR(VarDefVec, i, aliases)
        i->second->serializeAlias(serializer, i->first);
    
    // then the types
    for (int i = 0; i < privateStart; ++i)
        orderedTypes[i]->serialize(serializer, true, this);

    // ... then everything else    
    SPUG_FOR(vector<VarDef *>, i, others)
        (*i)->serialize(serializer, true, this);
    
    // now do the privates
    Serializer::StackFrame<Serializer> digestState(serializer, false);
    serializer.write(privateAliases.size() + 
                      orderedTypes.size() - privateStart +
                      otherPrivates.size(),
                     "#privateDefs"
                     );
    
    // aliases
    SPUG_FOR(VarDefVec, i, privateAliases)
        i->second->serializeAlias(serializer, i->first);
    
    // types
    for (int i = privateStart; i < orderedTypes.size(); ++i)
        orderedTypes[i]->serialize(serializer, true, this);
    
    // vars and functions
    SPUG_FOR(vector<VarDef *>, i, otherPrivates)
        (*i)->serialize(serializer, true, this);
}

void Namespace::deserializeTypeDecls(Deserializer &deser) {
    unsigned count = deser.readUInt("#decls");
    for (int i = 0; i < count; ++i)
        // This triggers the side-effect of populating the deserializer's 
        // object registry with an instance of the type.
        TypeDef::deserializeDecl(deser);
}

void Namespace::deserializeDefs(Deserializer &deser) {
    deserializeDefs(deser, "#defs", true);
    Serializer::StackFrame<Deserializer> digestState(deser, false);
    deserializeDefs(deser, "#privateDefs", false);
}

void Namespace::onNamespaceDeserialized(Context &context) {
    for (VarDefMap::iterator iter = defs.begin();
         iter != defs.end();
         ++iter
         ) {
        if (iter->second->getOwner() == this)
            iter->second->onDeserialized(context);
    }
}