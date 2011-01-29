// Copyright 2010 Google Inc.

#include "FuncAnnotation.h"

#include "compiler/CrackContext.h"
#include "parser/Parser.h"
#include "Context.h"
#include "FuncDef.h"

using namespace compiler;
using namespace model;
using namespace parser;

FuncAnnotation::FuncAnnotation(FuncDef *func) :
    Annotation(0, func->name), 
    func(func) {
}

void FuncAnnotation::invoke(Parser *parser, Toker *toker, Context *context) {
    typedef void (*AnnotationFunc)(CrackContext *);
    CrackContext crackCtx(parser, toker, context);
    
    // use the compile time construct's builder
    Construct *construct = context->getCompileTimeConstruct();
    ((AnnotationFunc)func->getFuncAddr(*construct->rootBuilder))(&crackCtx);
}
