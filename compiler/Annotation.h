// Copyright 2010 Google Inc.

#ifndef _crack_compiler_Annotation_h_
#define _crack_compiler_Annotation_h_

#include "ext/RCObj.h"

namespace model {
    class Annotation;
}

namespace compiler {

class Annotation : public crack::ext::RCObj {
    private:
        model::Annotation *rep;

    public:
        Annotation(model::Annotation *rep) : rep(rep) {}

        /**
         * Returns the annotation's user data.
         */
        void *getUserData();

        /**
         * Returns the annotation's name.
         */
        const char *getName();

        /**
         * Returns the annotations function.
         */
        void *getFunc();

};

} // namespace compiler

#endif
