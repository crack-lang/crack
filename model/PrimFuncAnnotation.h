// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

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
        
        void *getUserData() { return userData; }
        AnnotationFunc getFunc() { return func; }        
};

} // namespace model

#endif
