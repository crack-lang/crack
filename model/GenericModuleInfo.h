// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _model_GenericModuleInfo_h_
#define _model_GenericModuleInfo_h_

namespace model {

// Lets the module deserialization code pass back information on generic
// ephemeral modules even if they need to be recompiled.
class GenericModuleInfo {
    public:
        // True if generic module info was loaded.
        bool present;
        std::vector<std::string> name;
        std::vector<TypeDefPtr> params;
        std::string module;

        GenericModuleInfo() : present(false) {}
};

}

#endif
