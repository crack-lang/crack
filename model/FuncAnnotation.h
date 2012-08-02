// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
