// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
