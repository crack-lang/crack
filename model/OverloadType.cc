// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "OverloadType.h"

#include "spug/stlutil.h"

#include "builder/Builder.h"
#include "model/Context.h"
#include "model/VarDefImpl.h"
#include "model/OverloadDef.h"

#include "FuncDef.h"
#include "GenericOverloadType.h"

using namespace model;
using namespace std;

OverloadType::OverloadType(TypeDef *metaType, TypeDef *templateType,
                           TypeDef *builderType
                           ) :
    TypeDef(metaType, "Overload", false),
    builderType(builderType) {

    this->templateType = templateType;
    owner = this->templateType->getOwner();
    hasBuilderData = true;
    if (builderType)
        impl = builderType->impl;
}

TypeDefPtr OverloadType::getVarType() {
    if (genericParms.size() > 1)
        return 0;
    else
        return genericParms[0];
}

void OverloadType::addOperCall(TypeDef *source) {
    OverloadDefPtr ovld = lookUp("oper call");

    if (!ovld) {
        ovld = new OverloadDef("oper call");

        // XXX this ends up being a recursive definition, we're going to need
        // to deal with that.
        ovld->type = 0;

        // We don't have to collect ancestors, there are none.  Just add the
        // overload.
        addDef(ovld.get());
    }

    // Add the "oper call" methods from funcType.
    OverloadDefPtr operCall = source->lookUp("oper call");
    if (!operCall)
        // XXX builtins_need_oper_call - somehow we're getting in this code
        // for for classes.
        return;

    for(OverloadDef::FuncList::iterator iter = operCall->beginTopFuncs();
        iter != operCall->endTopFuncs();
        ++iter
        )
        ovld->addFunc(iter->get());
}

OverloadTypePtr OverloadType::addType(TypeDef *funcType) {

    // Ignore operations.
    if (!funcType)
        return this;

    // See if we've already got it.
    string name = funcType->getFullName();
    if (spug::contains(types, name))
        return this;

    // Construct a vector of types so that we can get a specialization of
    // OverloadType.  These are necessarily ordered by type name, which is
    // necessary so that we match any past instantiation of a generic with
    // this set of parameter types.
    TypeVec typeVec;
    typeVec.reserve(types.size() + 1);
    SPUG_FOR(TypeMap, i, types) {
        // Insert the new type at the appropriate location.
        if (name > i->first) {
            typeVec.push_back(funcType);
            funcType = 0;
        }
        typeVec.push_back(i->second);
    }

    // If the new type wasn't added in the loop, add it to the end.
    if (funcType)
        typeVec.push_back(funcType);

    TypeVecObjPtr tvo = new TypeVecObj(typeVec);
    return GenericOverloadTypePtr::cast(templateType)->getSpecialization(
        tvo.get()
    );
}

OverloadTypePtr OverloadType::addTypes(const list<FuncDefPtr> &funcs) {

    // Put all of the new types into an array.
    TypeVec typeVec;
    SPUG_FOR(list<FuncDefPtr>, i, funcs) {
        if ((*i)->type && !spug::contains(types, (*i)->type->getFullName()))
            typeVec.push_back((*i)->type.get());
    }

    // Quit if we've got nothing to add.
    if (typeVec.empty())
        return this;

    // Add all of the existing types and sort.
    SPUG_FOR(TypeMap, i, types)
        typeVec.push_back(i->second);

    TypeVecObjPtr tvo = new TypeVecObj(typeVec);
    return GenericOverloadTypePtr::cast(templateType)->getSpecialization(
        tvo.get()
    );
}
