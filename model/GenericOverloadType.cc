// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "GenericOverloadType.h"

#include <sstream>

#include "spug/stlutil.h"

#include "builder/Builder.h"
#include "model/Context.h"
#include "model/NullConst.h"
#include "model/VarDefImpl.h"
#include "OverloadType.h"

using namespace model;
using namespace std;

GenericOverloadType::GenericOverloadType(TypeDef *metaType,
                                         TypeDef *builderType
                                         ) :
    TypeDef(metaType, "Overload", false),
    builderType(builderType) {

    generic = new TypeDef::SpecializationCache();
    hasBuilderData = true;
    impl = builderType->impl;
}

OverloadTypePtr GenericOverloadType::getSpecialization(Context &context,
                                                       TypeVecObj *types
                                                       ) {
    OverloadTypePtr result;

    // Null types means we want to get the empty Overload.
    if (types) {
        result = OverloadTypePtr::cast(findSpecialization(types));
    } else {
        if (emptyOverload)
            return emptyOverload;

        // Fall through and create the empty overload.
        types = new TypeVecObj();
    }

    if (!result) {
        // Construct the type name.
        ostringstream tmp;
        bool first = true;
        tmp << "Overload[";
        SPUG_FOR(TypeDef::TypeVec, iter, *types) {
            if (first)
                first = false;
            else
                tmp << ", ";
            tmp << (*iter)->getFullName();
        }
        tmp << "]";

        result = new OverloadType(type.get(), this,
                                  context.builder.createGenericClass(context,
                                                                     tmp.str()
                                                                     ).get()
                                  );

        // Fill in the remaining fields.
        result->name = tmp.str();
        TypeVecObjPtr temp = types;
        result->genericParms = *types;
        result->templateType = this;
        result->defaultInitializer = new NullConst(result.get());
        (*generic)[TypeVecObjKey(types)] = result;
        if (types->empty())
            emptyOverload = result;

        // Add all of the types to the new types map and
        // add "oper call" methods for all of the types.
        SPUG_FOR(TypeDef::TypeVec, iter, *types) {
            result->types[(*iter)->getFullName()] = *iter;
            result->addOperCall(context, iter->get());
        }
    }

    return result;
}

TypeDefPtr GenericOverloadType::getSpecialization(Context &context,
                                                  TypeVecObj *types,
                                                  bool checkCache
                                                  ) {
    sort(types->begin(), types->end(), nameLessThan);
    return getSpecialization(context, types);
}
