// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_ContextStackFrame_h_
#define _model_ContextStackFrame_h_

#include "Context.h"

namespace model {

template <typename T>
class ContextStackFrame {
    private:
        bool restored;
        T &managed;
        model::ContextPtr context;

    public:
        ContextStackFrame(T &managed,
                          model::Context *context
                          ) :
            restored(false),
            managed(managed),
            context(managed.context) {

            managed.context = context;
        }

        ~ContextStackFrame() {
            if (!restored)
                restore();
        }

        void restore() {
            assert(!restored);
            managed.context = context.get();
            restored = true;
        }

        model::Context &parent() {
            assert(!restored);
            return *context;
        }
};


} // namespace model

#endif
