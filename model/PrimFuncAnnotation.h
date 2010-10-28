// Copyright 2010 Google Inc.

#ifndef _model_PrimFuncAnnotation_h_
#define _model_PrimFuncAnnotation_h_

#include "Annotation.h"

namespace parser {
    class Parser;
    class Toker;
}

namespace compiler {
    class CrackContext;
}

namespace model {

SPUG_RCPTR(PrimFuncAnnotation);
SPUG_RCPTR(PrimFuncDef);

/** 
 * An annotation that wraps a primitive function pointer.  These are also used 
 * for storing constructed annotations implemented in crack.
 */
class PrimFuncAnnotation : public Annotation {
    typedef void (*AnnotationFunc)(compiler::CrackContext *);
    private:
        AnnotationFunc func;
        void *userData;

    public:
        PrimFuncAnnotation(const std::string &name, AnnotationFunc func,
                           void *userData = 0
                           );
        virtual void invoke(parser::Parser *parser, parser::Toker *toker, 
                            Context *context
                            );
};

} // namespace model

#endif
