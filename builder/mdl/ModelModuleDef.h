// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _builder_model_MockModuleDef_h_
#define _builder_model_MockModuleDef_h_

#include "model/ModuleDef.h"

namespace builder { namespace mdl {

struct ModelModuleDef : public model::ModuleDef {

    ModelModuleDef(const std::string &name, Namespace *parent) :
        ModuleDef(name, parent) {
    }

    virtual void callDestructor() {}
    virtual void runMain(builder::Builder &builder) { }
};

}}
#endif

