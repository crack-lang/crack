// Copyright 2010 Google Inc.

#ifndef _model_FuncAnnotation_h_
#define _model_FuncAnnotation_h_

#include "Annotation.h"

namespace parser {
    class Parser;
    class Toker;
}

namespace model {

SPUG_RCPTR(FuncAnnotation);
SPUG_RCPTR(FuncDef);

/** An annotation implemented as a function. */
class FuncAnnotation : public Annotation {
    private:
        FuncDefPtr func;

    public:
        FuncAnnotation(FuncDef *func);
        virtual void invoke(parser::Parser *parser, parser::Toker *toker, 
                            Context *context
                            );
};

} // namespace model

#endif
