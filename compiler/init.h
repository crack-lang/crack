// Copyright 2010 Google Inc.
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
