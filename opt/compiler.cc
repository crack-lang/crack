
#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Func.h"
#include "compiler/init.h"

extern "C"
void crack_compiler_rinit() {
    return;
}

extern "C" void crack_compiler_cinit(crack::ext::Module *mod) {
    compiler::init(mod);
}

