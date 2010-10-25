// Copyright 2010 Google Inc.

#ifndef _model_VarAnnotation_h_
#define _model_VarAnnotation_h_

#include "Annotation.h"

namespace model {

SPUG_RCPTR(VarAnnotation);

/** An annotation implemented as a variable. */
class VarAnnotation : VarDef {
    private:
        VarDefPtr var;

    public:
        VarAnnotation(VarDef *func);
        virtual void invoke(Parser *parser, Toker *toker, Context *context);
};

} // namespace model

#endif
