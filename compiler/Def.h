// Copyright 2017 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _crack_compiler_Def_h_
#define _crack_compiler_Def_h_

#include <string>

#include "ext/RCObj.h"

namespace model {
    class VarDef;
}

namespace compiler {

/**
 * A definition linked-list node.  See CrackContext::getAllDefs().
 */
class Def : public crack::ext::RCObj {
    private:
        std::string localName;
        model::VarDef *rep;
        const Def *next;

    public:
        Def(const std::string &name, model::VarDef *rep, Def *next);

        ~Def();

        /**
         * Returns the simple name of the object.  Note that if the
         * definition is an alias, this returns the simple name of the aliased
         * object, not the alias itself.  Use getLocalName() to get the alias.
         */
        const char *getName() const;

        /**
         * Returns the fully qualified name of the object.
         *
         * Note that if the definitition is an alias, this returns the full
         * name of the aliased object, not the name of the alias.
         */
        const char *getFullName() const;

        /**
         * Returns the local name of the definition.  This returns the alias if
         * the definition is an alias, the object name otherwise.
         */
        const char *getLocalName() const;

        /**
         * Return the next node in the linked list, null if there are none.
         */
        const Def *getNext() const {
            return next;
        }

        static const char *_getName(const Def *inst);
        static const char *_getFullName(const Def *inst);
        static const char *_getLocalName(const Def *inst);
        static const Def *_getNext(const Def *inst);
        static void _bind(Def *inst);
        static void _release(Def *inst);
};

} // namespace compiler

#endif
