// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "ConstVarDef.h"

#include "spug/check.h"
#include "builder/Builder.h"
#include "Deserializer.h"
#include "FloatConst.h"
#include "IntConst.h"
#include "Serializer.h"

using namespace model;
using namespace std;

namespace {
    enum ConstType {
        constInt = 1,
        constIntReqUnsigned = 2,
        constFloat = 3
    };
}

void ConstVarDef::serialize(Serializer &serializer, bool writeKind,
                            const Namespace *ns
                            ) const {
    if (writeKind)
        serializer.write(Serializer::constVarId, "kind");

    // write the name
    serializer.write(name, "name");

    // write the type
    type->serialize(serializer, false, 0);

    if (IntConst *val = IntConstPtr::rcast(expr)) {
        serializer.write(val->reqUnsigned ? constIntReqUnsigned :
                                            constInt,
                         "kind"
                         );
        serializer.write(val->val.uval, "val");
    } else if (FloatConst *val = FloatConstPtr::rcast(expr)) {
        serializer.write(constFloat, "kind");
        serializer.writeDouble(val->val, "val");
    } else {
        SPUG_CHECK(false, "Invalid const var expression: " << expr);
    }
}

ConstVarDefPtr ConstVarDef::deserialize(Deserializer &deser) {
    string name = deser.readString(16, "name");
    TypeDefPtr type = TypeDef::deserializeRef(deser, "type");
    int kind = deser.readUInt("kind");
    bool reqUnsigned = false;
    IntConstPtr intVal;
    double doubleVal;
    ExprPtr expr;
    switch (kind) {
        case constIntReqUnsigned:
            reqUnsigned = true;
        case constInt:
            expr = intVal = deser.context->builder.createIntConst(
                *deser.context,
                static_cast<uint64_t>(deser.readUInt("val")),
                type.get()
            );
            intVal->reqUnsigned = reqUnsigned;
            break;
        case constFloat:
            doubleVal = deser.readDouble("val");
            expr = deser.context->builder.createFloatConst(
                *deser.context,
                doubleVal,
                type.get()
            );
            break;
        default:
            SPUG_CHECK(false, "unexpected const var def kind: " << kind);
    }

    return new ConstVarDef(type.get(), name, expr);
}
