// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 
// contains the prototype for the init function of the built-in crack.compiler 
// extension module.

#ifndef _compiler_init_h_
#define _compiler_init_h_

namespace crack { namespace ext {
    class Module;
}}

namespace compiler {

void init(crack::ext::Module *mod);

}

#endif
