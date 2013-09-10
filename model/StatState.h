// Copyright 2013 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_StatState_h_
#define _model_StatState_h_

#include "spug/RCPtr.h"
#include "Construct.h" // for ConstructStats, which belongs in its own file.

namespace model {

SPUG_RCPTR(Context);

/**
 * a sentinel class for keeping track of ConstructStats state
 * it will set a new state (and optionally module) on construction, and restore
 * both on destruction. if stats are disabled in options, it will ignore the
 * calls.
 */
class StatState {
        ContextPtr context;
        ConstructStats::CompileState oldState;
        model::ModuleDefPtr oldModule;

    public:

        StatState(Context *c, ConstructStats::CompileState newState);

        StatState(Context *c,
                  ConstructStats::CompileState newState,
                  model::ModuleDef *newModule
                  );

        bool statsEnabled(void);

        ~StatState();
};

} // namespace model

#endif
