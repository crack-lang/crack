// Copyright 2010 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_StubDef_h
#define _model_StubDef_h

#include <spug/RCPtr.h>

#include "VarDef.h"

namespace model {

SPUG_RCPTR(StubDef);

/**
 * When we import from a shared library, we end up with an address but no
 * definition.  A StubDef stores the address and reserves the name in the
 * namespace until a definition arrives.
 */
class StubDef : public VarDef {
    public:

        void *address;

        /**
         * @param type should be the "void" type.
         */
        StubDef(TypeDef *type, const std::string &name, void *address) :
            VarDef(type, name),
            address(address) {
        }

        virtual bool hasInstSlot() const { return false; }
};

} // namespace model


#endif
