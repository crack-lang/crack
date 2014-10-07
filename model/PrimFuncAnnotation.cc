// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "PrimFuncAnnotation.h"

#include "compiler/CrackContext.h"
#include "model/TypeDef.h"

using namespace model;

PrimFuncAnnotation::PrimFuncAnnotation(const std::string &name,
                                       AnnotationFunc func,
                                       void *userData
                                       ) : 
    Annotation(0, name),
    func(func),
    userData(userData) {
}

void PrimFuncAnnotation::invoke(parser::Parser *parser, parser::Toker *toker, 
                                Context *context
                                ) {
    compiler::CrackContext crackCtx(parser, toker, context, userData);
    func(&crackCtx);
}
