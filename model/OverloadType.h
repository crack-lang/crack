// Copyright 2016 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_OverloadType_h
#define _model_OverloadType_h

#include <list>

#include "TypeDef.h"

namespace model {

SPUG_RCPTR(FuncDef);

SPUG_RCPTR(OverloadType);

/**
 * Overload types aggregate the types of all of the functions contained in an
 * overload.  They should be treated as immutable after construction.  When
 * an overload adds a new function, it creates a new type.  This preserves
 * all of the normal expectations of types (that they can be compared by
 * identity).  That said, an OverloadType can share the function types of the
 * overloads that it is based on.
 */
class OverloadType : public TypeDef {
    private:

        TypeDefPtr builderType;

        // The function types that this overload represents.
        typedef std::map<std::string, TypeDefPtr> TypeMap;
        TypeMap types;

    public:
        OverloadType(TypeDef *metaType, TypeDef *templateType,
                     TypeDef *builderType);

        TypeDefPtr getVarType();

        /**
         * Aliases the "oper call" from 'source' into the type's namespace.
         */
        void addOperCall(TypeDef *source);

        /**
         * Add a new function type to the overload def type and returns a new
         * OverloadType containing the new type.  This gets called when a
         * function is added to the overload.
         */
        OverloadTypePtr addType(TypeDef *funcType);

        /**
         * Add all types in the list to the overload def type and returns a
         * new OverloadType for the resulting set of types.
         */
        OverloadTypePtr addTypes(const std::list<FuncDefPtr> &funcs);

        /**
         * Returns the builder type for the type.
         */
        TypeDefPtr getBuilderType() {
            return builderType;
        }
};

}

#endif

