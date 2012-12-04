// Copyright 2010,2012 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2010-2012 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 


#include "Namespace.h"

#include "spug/check.h"
#include "Context.h"
#include "Deserializer.h"
#include "Expr.h"
#include "OverloadDef.h"
#include "Serializer.h"
#include "VarDef.h"

using namespace std;
using namespace model;

void Namespace::storeDef(VarDef *def) {
    assert(!FuncDefPtr::cast(def) && 
           "it is illegal to store a FuncDef directly (should be wrapped "
           "in an OverloadDef)");
    defs[def->name] = def;
    orderedForCache.push_back(def);
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

ModuleDefPtr Namespace::getRealModule() {
    ModuleDefPtr mod = getModule();
    ModuleDefPtr owner = ModuleDefPtr::cast(mod->getOwner());
    return owner ? owner : mod;
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

void Namespace::replaceDef(VarDef *def) {
    assert(!def->getOwner());
    assert(!def->hasInstSlot() && 
           "Attempted to replace an instance variable, this doesn't work "
           "because it won't change the 'ordered' vector."
           );
    def->setOwner(this);
    defs[def->name] = def;
}

void Namespace::dump(ostream &out, const string &prefix) {
    out << canonicalName << " (0x" << this << ") {\n";
    string childPfx = prefix + "  ";
    unsigned i = 0;
    Namespace *parent;
    while (parent = getParent(i++).get()) {
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

void Namespace::dump() {
    dump(cerr, "");
}

void Namespace::serializeDefs(Serializer &serializer) const {
    
    // count the number of definitions to serialize
    int count = 0;
    for (VarDefMap::const_iterator i = defs.begin();
         i != defs.end();
         ++i
         ) {
        if (i->second->isSerializable())
            ++count;
    }
    
    // write the count and the definitions
    serializer.write(count, "#defs");
    for (VarDefMap::const_iterator i = defs.begin();
         i != defs.end();
         ++i
         ) {
        if (!i->second->isSerializable())
            continue;
        else if (i->second->getModule() != serializer.module)
            i->second->serializeAlias(serializer, i->first);
        else
            i->second->serialize(serializer, true);
    }
}

void Namespace::deserializeDefs(Deserializer &deser) {
    // read all of the symbols
    unsigned count = deser.readUInt("#defs");
    for (int i = 0; i < count; ++i) {
        int kind = deser.readUInt("kind");
        switch (static_cast<Serializer::DefTypes>(kind)) {
            case Serializer::variableId:
                addDef(VarDef::deserialize(deser).get());
                break;
            case Serializer::aliasId: {
                string alias = 
                    deser.readString(Serializer::varNameSize, "alias");
                addAlias(alias, VarDef::deserializeAlias(deser).get());
                break;
            }
            case Serializer::genericId:
                // XXX don't think we need this, generics are probably stored 
                // in a type.
                SPUG_CHECK(false, "can't deserialize generics yet");
//                addDef(Generic::deserialize(deser));
                break;
            case Serializer::overloadId:
                addDef(OverloadDef::deserialize(deser).get());
                break;
            case Serializer::typeId:
                TypeDef::deserialize(deser).get();
                break;
            default:
                SPUG_CHECK(false, "Bad definition type id " << kind);
        }
    }
}
