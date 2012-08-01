// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
