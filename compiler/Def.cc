// Copyright 2017 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "Def.h"

#include "model/TypeDef.h"
#include "model/VarDef.h"

using namespace compiler;
using namespace model;

Def::Def(const std::string &name, VarDef *rep, Def *next) :
    localName(name),
    rep(rep),
    next(next) {

    rep->incref();
    if (next)
        next->bind();
}

Def::~Def() {
    rep->decref();
    if (next)
        const_cast<Def*>(next)->release();
}

const char *Def::getName() const {
    return rep->name.c_str();
}

const char *Def::getFullName() const {
    return rep->getFullName().c_str();
}

const char *Def::getLocalName() const {
    return localName.c_str();
}

const char *Def::_getName(const Def *inst) {
    return inst->getName();
}

const char *Def::_getFullName(const Def *inst) {
    return inst->getFullName();
}

const char *Def::_getLocalName(const Def *inst) {
    return inst->getLocalName();
}

const Def *Def::_getNext(const Def *inst) {
    return inst->getNext();
}

void Def::_bind(Def *inst) {
    if (inst)
        inst->bind();
}

void Def::_release(Def *inst) {
    if (inst)
        inst->release();
}

