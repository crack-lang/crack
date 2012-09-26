// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef _crack_compiler_Annotation_h_
#define _crack_compiler_Annotation_h_

#include "ext/RCObj.h"
#include "init.h"

namespace crack { namespace ext {
    class Module;
}}

namespace model {
    class Annotation;
}

namespace compiler {

class Annotation : public crack::ext::RCObj {
    friend void compiler::init(crack::ext::Module *mod);
    private:
        model::Annotation *rep;
        
        static void *_getUserData(Annotation *inst);
        static const char *_getName(Annotation *inst);
        static void *_getFunc(Annotation *inst);

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
