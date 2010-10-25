// Copyright 2010 Google Inc.

#ifndef _model_Annotation_h_
#define _model_Annotation_h_

#include "spug/RCPtr.h"
#include "VarDef.h"

namespace parser {
    class Parser;
    class Toker;
}

namespace model {

class Context;
class VarDef;

SPUG_RCPTR(Annotation);

/**
 * Annotation is a wrapper around either a variable or a function that makes
 * using it easier for the parser.
 */
class Annotation : public VarDef {
    public:
        Annotation(model::TypeDef *type, const std::string &name);
        virtual void invoke(parser::Parser *parser, parser::Toker *toker, 
                            model::Context *context) = 0;
        
        /** 
         * Create a new annotation from the definition of the correct derived 
         * type (if 'def' is a function, creates a FuncAnnotation, if it is a 
         * variable creates a VarAnnotation)
         */
        static AnnotationPtr create(VarDef *def);
};

} // namespace model

#endif
