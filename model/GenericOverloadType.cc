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
                                         TypeDef *builderType,
                                         Context &context
                                         ) :
    TypeDef(metaType, "Overload", false),
    context(&context),
    builderType(builderType) {

    generic = new TypeDef::SpecializationCache();
    hasBuilderData = true;
    impl = builderType->impl;
}

OverloadTypePtr GenericOverloadType::getSpecialization(TypeVecObj *types) {
    OverloadTypePtr result;
    if (types)
        result = OverloadTypePtr::cast(findSpecialization(types));
    if (!result) {

        // Create an empty type vec if we didn't get one.
        if (!types)
            types = new TypeVecObj();

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
                                  context->builder.createGenericClass(*context,
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

        // Add "oper call" methods for all of the types.
        SPUG_FOR(TypeDef::TypeVec, iter, *types)
            result->addOperCall(iter->get());
    }

    return result;
}

namespace {
    bool nameLessThan(const TypeDefPtr &a, const TypeDefPtr &b) {
        return a->getFullName() < b->getFullName();
    }
}

TypeDefPtr GenericOverloadType::getSpecialization(Context &context,
                                                  TypeVecObj *types,
                                                  bool checkCache
                                                  ) {
    sort(types->begin(), types->end(), nameLessThan);
    return getSpecialization(types);
}
