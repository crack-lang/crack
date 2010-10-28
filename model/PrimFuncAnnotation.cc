// Copyright 2010 Google Inc.

#include "PrimFuncAnnotation.h"

#include "compiler/CrackContext.h"

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
