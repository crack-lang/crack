// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _crack_tests_MockFuncDef_h_
#define _crack_tests_MockFuncDef_h_

#include "model/FuncDef.h"

struct MockFuncDef : public model::FuncDef {
    MockFuncDef(Flags flags, const std::string &name, size_t argCount) :
        FuncDef(flags, name, argCount) {
    }

    virtual void *getFuncAddr(builder::Builder &builder) {
        return 0;
    }
};

#endif
