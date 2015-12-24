// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_DummyModuleDef_h_
#define _model_DummyModuleDef_h_

#include "ModuleDef.h"

namespace builder {
    class Builder;
}

namespace model {

/**
 * A dummy module implementation.  This is useful as a placeholder in certain
 * cases.
 */
class DummyModuleDef : public ModuleDef {
    public:
        DummyModuleDef(const std::string &name, Namespace *ns) :
            ModuleDef(name, ns) {
        }
        virtual void callDestructor() {}
        virtual void runMain(builder::Builder &builder) {}
        virtual bool isHiddenScope() { return true; }
};

}

#endif

