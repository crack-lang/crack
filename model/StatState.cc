// Copyright 2013 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "StatState.h"
#include "builder/Builder.h"

using namespace model;

StatState::StatState(Context *c, ConstructStats::CompileState newState) :
    context(c) {

    if (!context->construct->rootBuilder->options->statsMode)
        return;
    oldState = context->construct->stats->getState();
    context->construct->stats->setState(newState);
}

StatState::StatState(Context *c,
                     ConstructStats::CompileState newState,
                     model::ModuleDef *newModule) :
    context(c) {

    if (!context->construct->rootBuilder->options->statsMode)
        return;
    oldState = context->construct->stats->getState();
    oldModule = context->construct->stats->getModule().get();
    context->construct->stats->setState(newState);
    context->construct->stats->setModule(newModule);
}

bool StatState::statsEnabled(void) {
    return context->construct->rootBuilder->options->statsMode;
}

StatState::~StatState() {
    if (!context->construct->rootBuilder->options->statsMode)
        return;
    context->construct->stats->setState(oldState);
    if (oldModule)
        context->construct->stats->setModule(oldModule.get());
}
