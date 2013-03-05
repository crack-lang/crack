// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _crack_tests_MockModuleDef_h_
#define _crack_tests_MockModuleDef_h_

#include "model/ModuleDef.h"

struct MockModuleDef : public model::ModuleDef {

    MockModuleDef(const std::string &name, Namespace *parent) :
        ModuleDef(name, parent) {
    }

    virtual void callDestructor() {}
    virtual void runMain(builder::Builder &builder) { }
};


#endif

