// Copyright 2010-2011 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Location.h"

#include "parser/Location.h"

using namespace compiler;
using namespace crack::ext;

Location::Location(const parser::Location &loc) {
    rep = new parser::Location(loc);
}

Location::~Location() {
    delete rep;
}

const char *Location::getName() {
    return rep->getName();
}

int Location::getLineNumber() {
    return rep->getLineNumber();
}

const char *Location::_getName(Location *inst) {
    return inst->rep->getName();
}

int Location::_getLineNumber(Location *inst) {
    return inst->rep->getLineNumber();
}

void Location::_bind(Location *inst) { inst->bind(); }
void Location::_release(Location *inst) { inst->release(); }

