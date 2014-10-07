// Copyright 2010-2011 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "compiler/Annotation.h"

#include "model/FuncAnnotation.h"
#include "model/PrimFuncAnnotation.h"
#include "model/TypeDef.h"

using namespace compiler;

void *Annotation::getUserData() {
    model::PrimFuncAnnotation *pfa = model::PrimFuncAnnotationPtr::cast(rep);
    if (pfa)
        return pfa->getUserData();
    else
        return 0;
}

const char *Annotation::getName() {
    return rep->name.c_str();
}

void *Annotation::getFunc() {
    model::PrimFuncAnnotation *pfa = model::PrimFuncAnnotationPtr::cast(rep);
    if (pfa)
        return reinterpret_cast<void *>(pfa->getFunc());
    else
        return 0;
}

void *Annotation::_getUserData(Annotation *inst) {
    model::PrimFuncAnnotation *pfa =
        model::PrimFuncAnnotationPtr::cast(inst->rep);
    if (pfa)
        return pfa->getUserData();
    else
        return 0;
}

const char *Annotation::_getName(Annotation *inst) {
    return inst->rep->name.c_str();
}

void *Annotation::_getFunc(Annotation *inst) {
    model::PrimFuncAnnotation *pfa =
        model::PrimFuncAnnotationPtr::cast(inst->rep);
    if (pfa)
        return reinterpret_cast<void *>(pfa->getFunc());
    else
        return 0;
}
