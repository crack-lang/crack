// Copyright 2010 Google Inc.

#include "Annotation.h"

#include "Expr.h"
#include "FuncAnnotation.h"
#include "FuncDef.h"

using namespace model;

Annotation::Annotation(model::TypeDef *type, const std::string &name) :
    VarDef(type, name) {
    
}

AnnotationPtr Annotation::create(VarDef *def) {
    FuncDef *funcDef;
    if (funcDef = FuncDefPtr::cast(def))
        return new FuncAnnotation(funcDef);
//    else
//        return new VarAnnotation(def);
    assert(false && "VarAnnotations not supported yet");
}
