// Copyright 2010 Google Inc.

#include "compiler/Annotation.h"

#include "model/FuncAnnotation.h"
#include "model/PrimFuncAnnotation.h"

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
