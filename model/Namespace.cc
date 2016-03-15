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
#include "NamespaceAliasTreeNode.h"
#include "OverloadAliasTreeNode.h"
#include "OverloadDef.h"
#include "ProtoBuf.h"
#include "Serializer.h"
#include "StubDef.h"
#include "VarDef.h"

using namespace spug;
using namespace std;
using namespace model;

void Namespace::OrderedTypes::add(const TypeDef *type,
                                  const ModuleDef *master
                                  ) {
    // We can quit now if:
    // 1) We've already got the type in the collection.  
    // 2) We find a type outside of the copseristence group (since we know that 
    //    all cycles must be contained to a copersistence group).
    // 3) The type is not serializable.  (Serializable types are very special 
    //    [meta-types and internal types whose names start with ':'] so we can 
    //    safely assume that none of their dependencies are serializable 
    //    either.
    if (contains(type) || 
        const_cast<TypeDef *>(type)->getModule()->getMaster() != master ||
        !type->isSerializable()
        )
        return;
    
    // Do the base classes.
    for (int i = 0; i < type->parents.size(); ++i)
        add(type->parents[i].get(), master);
        
    // If this is a generic instantiation, do the generic.
    if (type->templateType)
        add(type->templateType, master);

    // Finally, add the type to both sub-collections.
    ordered.push_back(type);
    indexed.insert(type);
}

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
            if (def->getOwner() == this && def->isSerializable())
                typeDefs.push_back(def);
        } else {
            SPUG_CHECK(!NamespacePtr::rcast(iter->second), 
                       "found a non-type namespace: " << iter->first
                       );
        }
    }
    getNestedTypeDefs(typeDefs, master);
}

namespace {
    // Add an overload during deserialization.
    void addOverload(Namespace *ns, OverloadDef *ovld) {
        // Since overloads can be serialized more than once to deal
        // with aliases, we only want to add it if it's not already
        // registered.
        if (!ovld->getOwner())
            ns->addDef(ovld);
        else
            SPUG_CHECK(ovld->getOwner() == ns,
                       "Reusing overload that is not owned by " <<
                        ns->getNamespaceName()
                       );
    }
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
            case Serializer::overloadId:
                addOverload(this,
                            OverloadDef::deserialize(deser, this, false).get()
                            );
                break;
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
        // avoid aliases
        if (isAlias(i->second.get(), i->first))
            continue;
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

bool Namespace::isAlias(const VarDef *def, const string &name) const {
    return def->getOwner() != this || name != def->name;
}

void Namespace::addDef(VarDef *def) {
    assert(!def->getOwner());

    storeDef(def);
    def->setOwner(this);
}

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
        OverloadDefPtr child = overload->createAlias(false);
        storeDef(child.get());
        child->setOwner(this);
    } else {
        storeDef(def);
    }
}

OverloadDefPtr Namespace::addAlias(const string &name, VarDef *def) {
    // make sure that the symbol is already bound to a context.
    Namespace *owner = def->getOwner();
    assert(owner);

    // See if the alias name exposes any private members of the overload.
    bool exposes = def->isImportable(this, name);

    // overloads should never be aliased - otherwise the new context could 
    // extend them.
    OverloadDef *overload = OverloadDefPtr::cast(def);
    if (overload) {
        OverloadDefPtr child = overload->createAlias(exposes);
        
        // Since we own the overload, we can rename it.
        child->name = name;
        
        defs[name] = child.get();
        child->setOwner(this);
        return child;
    } else {
        defs[name] = def;
        
        // See if the alias exposes a private def.
        if (exposes && !def->isImportable(owner, def->name))
            def->exposed = true;

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
            addAlias(iter->first, iter->second.get());
    
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

void Namespace::getOrderedTypes(OrderedTypes &types, const ModuleDef *master) const {
    SPUG_FOR(VarDefMap, iter, defs) {
        if (TypeDef *type = TypeDefPtr::rcast(iter->second)) {
            if (isAlias(iter->second.get(), iter->first))
                continue;
            types.add(type, master);
            type->getOrderedTypes(types, master);
        }
    }
}

void Namespace::serializeTypeDecls(Serializer &serializer, ModuleDef *master) {
    // We build a vector so we can determine the count up front.
    vector<TypeDef *> typeDefs;
    getTypeDefs(typeDefs, master);
    
    serializer.write(typeDefs.size(), "#decls");
    for (vector<TypeDef *>::const_iterator iter = typeDefs.begin();
         iter != typeDefs.end();
         ++iter
         ) {
        (*iter)->serializeDecl(serializer, master);
    }
}

void Namespace::serializeNonTypeDefs(const vector<const Namespace *>& namespaces, 
                                     Serializer &serializer
                                     ) const {
    // Count the number of definitions to serialize and separate out the 
    // types, other defs, and aliases.
    vector<VarDef *> defs, privateDefs;
    SPUG_FOR(vector<const Namespace *>, ns, namespaces) {
        // If the namespace has generics, we need to serialize private 
        // definitions.
        bool serializePrivates = (*ns)->hasGenerics();
        
        SPUG_FOR(VarDefMap, i, (*ns)->defs) {
            if (i->second->isSerializable()) {
                
                // is it an alias?
                if (isAlias(i->second.get(), i->first))
                    continue;
                
                // Is it an overload?
                if (OverloadDefPtr ovld = OverloadDefPtr::rcast(i->second)) {

                    // Ignore if the overload contains only aliases.
                    if (!ovld->hasNonAliases())
                        continue;

                    if (i->second->isImportable(*ns, i->first) || 
                        ovld->hasExposedFuncs()
                        )
                        defs.push_back(i->second.get());
                    else if (serializePrivates)
                        privateDefs.push_back(i->second.get());
                    continue;
                }
    
                if (!TypeDefPtr::rcast(i->second)) {
                    if (i->second->isImportable(*ns, i->first) || 
                        i->second->exposed
                        ) {
                        defs.push_back(i->second.get());
                    }
                    else if (serializePrivates) {
                        privateDefs.push_back(i->second.get());
                    }
                }
            }
        }
    }

    // write the count and the definitions
    serializer.write(defs.size(), "#defs");
    SPUG_FOR(vector<VarDef *>, i, defs)
        (*i)->serialize(serializer, true, this);
    
    // now do the privates
    {
        Serializer::StackFrame<Serializer> digestState(serializer, false);
        serializer.write(privateDefs.size(), "#privateDefs");

        // vars and functions
        SPUG_FOR(vector<VarDef *>, i, privateDefs)
            (*i)->serialize(serializer, true, this);
    }

    serializer.write(0, "optional");
}

void Namespace::deserializeTypeDecls(Deserializer &deser) {
    unsigned count = deser.readUInt("#decls");
    for (int i = 0; i < count; ++i)
        // This triggers the side-effect of populating the deserializer's 
        // object registry with an instance of the type.
        TypeDef::deserializeDecl(deser);
}

void Namespace::deserializeAliases(Deserializer &deser) {
    if (Serializer::trace)
        cerr << "# begin namespace " << getNamespaceName() << endl;

    int count = deser.readUInt("#children");
    for (int i = 0; i < count; ++i) {
        int kind = deser.readUInt("kind");
        if (kind == Serializer::slaveModuleId) {
            string name = deser.readString(Serializer::modNameSize, "name");
            ModuleDefPtr mod = deser.context->construct->getModule(name);
            mod->deserializeAliases(deser);
        } else if (kind == Serializer::typeId) {
            string name = deser.readString(Serializer::varNameSize, "name");
            TypeDefPtr type = lookUp(name, false);
            type->deserializeAliases(deser);
        } else if (kind == Serializer::overloadId) {
            addOverload(this, OverloadDef::deserialize(deser, this, true).get());
        }
    }

    count = deser.readUInt("#defs");
    for (int i = 0; i < count; ++i) {
        int kind = deser.readUInt("kind");
        string name = deser.readString(Serializer::varNameSize, "alias");
        VarDefPtr varDef;
        switch (kind) {
            case Serializer::aliasId:
                varDef = VarDef::deserializeAlias(deser);
                break;
            case Serializer::typeAliasId:
                varDef = TypeDef::deserializeRef(deser);
                break;
            default:
                SPUG_CHECK(false,
                           "Bad definition type id " << kind
                           );
        }
        addAlias(name, varDef.get());
    }

    if (Serializer::trace)
        cerr << "# end namespace " << getNamespaceName() << endl;
}

void Namespace::deserializeDefs(Deserializer &deser) {
    deserializeDefs(deser, "#defs", true);
    {
        Serializer::StackFrame<Deserializer> digestState(deser, false);
        deserializeDefs(deser, "#privateDefs", false);
    }
    deser.readString(64, "optional");
}

namespace {
    // These implement the check for whether an alias or overload should be
    // included in a specific type of alias tree (private or importable).

    bool shouldInclude(bool privateAliases, Namespace *ns,
                       Namespace::VarDefMap::const_iterator entry
                       ) {
        bool importable = entry->second->isImportable(ns, entry->first);
        if (privateAliases)
            return !importable;
        else
            return importable;
    }
}

AliasTreeNodePtr Namespace::getAliasTree(bool privateAliases) {
    NamespaceAliasTreeNodePtr result;
    SPUG_FOR(VarDefMap, i, defs) {
        if (isAlias(i->second.get(), i->first)) {
            if (shouldInclude(privateAliases, this, i)) {
                if (!result)
                    result = new NamespaceAliasTreeNode(this);
                result->addAlias(i->first, i->second.get());
            }
        } else if (OverloadDef *ovld = OverloadDefPtr::rcast(i->second)) {
            if (ovld->containsAliases(privateAliases)) {
                AliasTreeNodePtr childNode = ovld->getAliasTree(privateAliases);
                if (childNode) {
                    if (!result)
                        result = new NamespaceAliasTreeNode(this);
                    result->addChild(childNode.get());
                }
            }
        } else if (Namespace *child = NamespacePtr::rcast(i->second)) {
            AliasTreeNodePtr childNode = child->getAliasTree(privateAliases);
            if (childNode) {
                if (!result)
                    result = new NamespaceAliasTreeNode(this);
                result->addChild(childNode.get());
            }
        }
    }

    return result;
}

bool Namespace::isScopedTo(Namespace *other) {
    if (this == other)
        return true;
    else if (NamespacePtr owner = getNamespaceOwner())
        return owner->isScopedTo(other);
    else
        return false;
}
