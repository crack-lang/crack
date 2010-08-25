// Copyright 2010 Google Inc, Shannon Weyrick <weyrick@mozek.us>

#include "BFuncDef.h"
#include "BTypeDef.h"

#include "model/Context.h"
#include "model/OverloadDef.h"
#include "PlaceholderInstruction.h"

using namespace model;
using namespace std;
using namespace builder::mvll;

// add all of my virtual functions to 'vtb'
void BTypeDef::extendVTables(VTableBuilder &vtb) {
    // find all of the virtual functions
    for (Namespace::VarDefMap::iterator varIter = beginDefs();
         varIter != endDefs();
         ++varIter
         ) {

        BFuncDef *funcDef = BFuncDefPtr::rcast(varIter->second);
        if (funcDef && (funcDef->flags & FuncDef::virtualized)) {
            vtb.add(funcDef);
            continue;
        }

        // check for an overload (if it's not an overload, assume that
        // it's not a function).  Iterate over all of the overloads at
        // the top level - the parent classes have
        // already had their shot at extendVTables, and we don't want
        // their overloads to clobber ours.
        OverloadDef *overload =
            OverloadDefPtr::rcast(varIter->second);
        if (overload)
            for (OverloadDef::FuncList::iterator fiter =
                  overload->beginTopFuncs();
                 fiter != overload->endTopFuncs();
                 ++fiter
                 )
                if ((*fiter)->flags & FuncDef::virtualized)
                    vtb.add(BFuncDefPtr::arcast(*fiter));
    }
}

/**
 * Create all of the vtables for 'type'.
 *
 * @param vtb the vtable builder
 * @param name the name stem for the VTable global variables.
 * @param vtableBaseType the global vtable base type.
 * @param firstVTable if true, we have not yet discovered the first
 *  vtable in the class schema.
 */
void BTypeDef::createAllVTables(VTableBuilder &vtb, const string &name,
                      BTypeDef *vtableBaseType,
                      bool firstVTable
                                         ) {

    // if this is VTableBase, we need to create the VTable.
    // This is a special case: we should only get here when
    // initializing VTableBase's own vtable.
    if (this == vtableBaseType)
        vtb.createVTable(this, name, true);

    // iterate over the base classes, construct VTables for all
    // ancestors that require them.
    for (TypeVec::iterator baseIter = parents.begin();
         baseIter != parents.end();
         ++baseIter
         ) {
        BTypeDef *base = BTypeDefPtr::arcast(*baseIter);

        // if the base class is VTableBase, we've hit bottom -
        // construct the initial vtable and store the first vtable
        // type if this is it.
        if (base == vtableBaseType) {
            vtb.createVTable(this, name, firstVTable);

            // otherwise, if the base has a vtable, create all of its
            // vtables
        } else if (base->hasVTable) {
            if (firstVTable)
                base->createAllVTables(vtb, name, vtableBaseType,
                                       firstVTable
                                       );
            else
                base->createAllVTables(vtb,
                                       name + ':' + base->getFullName(),
                                       vtableBaseType,
                                       firstVTable
                                       );
        }

        firstVTable = false;
    }

    // we must either have ancestors with vtables or be vtable base.
    assert(!firstVTable || this == vtableBaseType);

    // add my functions to their vtables
    extendVTables(vtb);
}

void BTypeDef::addBaseClass(BTypeDef *base) {
    ++fieldCount;
    parents.push_back(base);
    if (base->hasVTable)
        hasVTable = true;
}

void BTypeDef::addPlaceholder(PlaceholderInstruction *inst) {
    assert(!complete && "Adding placeholder to a completed class");
    placeholders.push_back(inst);
}

BTypeDef *BTypeDef::findFirstVTable(BTypeDef *vtableBaseType) {

    // special case - if this is VTableBase, it is its own first vtable 
    // (normally it is the first class to derive from VTableBase that is the 
    // first vtable).
    if (this == vtableBaseType)
        return this;

    // check the parents
    for (TypeVec::iterator parent = parents.begin();
         parent != parents.end();
         ++parent
         )
        if (parent->get() == vtableBaseType) {
            return this;
        } else if ((*parent)->hasVTable) {
            BTypeDef *par = BTypeDefPtr::arcast(*parent);
            return par->findFirstVTable(vtableBaseType);
        }

    cerr << "class is " << name << endl;
    assert(false && "Failed to find first vtable");
}