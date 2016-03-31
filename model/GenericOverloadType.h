// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_GenericOverloadType_h
#define _model_GenericOverloadType_h

#include <algorithm>

#include "TypeDef.h"

namespace model {

class Context;
SPUG_RCPTR(OverloadType);

SPUG_RCPTR(GenericOverloadType);

/**
 * Manages the specialization cache of all OverloadTypes.
 * OverloadTypes are special in a couple of ways.  There is no code generated
 * for them.  The set of types that specialize them is also a set rather than
 * a vector, so calls to getSpecialization() should be provided with a type
 * vector sorted by type name.
 */
class GenericOverloadType : public TypeDef {
    private:
        TypeDefPtr builderType;
        model::ContextPtr context;

    public:
        GenericOverloadType(TypeDef *metaType, TypeDef *builderType,
                            model::Context &context
                            );

        /**
         * Gets an overload type specialization.  'types' must be ordered by
         * name.
         */
        OverloadTypePtr getSpecialization(TypeVecObj *types = 0);

        /**
         * This is provided just to be righteous, overloads will generally use
         * the non-virtual getSpecialization method above.
         */
        virtual TypeDefPtr getSpecialization(Context &context,
                                             TypeVecObj *types,
                                             bool checkCache
                                             );

        /**
         * Utility function for classes that need to sort by type names.
         */
        static bool nameLessThan(const TypeDefPtr &a, const TypeDefPtr &b) {
            return a->getFullName() < b->getFullName();
        }

        /**
         * Lets the builder store a BTypeDef object.  See TypeDef::hasImpl for
         * notes on this.
         */
        TypeDefPtr getBuilderType() {
            return builderType;
        }
};

}

#endif
