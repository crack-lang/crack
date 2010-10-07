
#include "ext/Func.h"
#include "ext/Module.h"

using namespace crack::ext;

const char *echo(const char *data) { return data; }

extern "C" void test_testext_init(Module *mod) {
    Func *f = mod->addFunc(mod->getByteptrType(), "echo", (void *)echo);
    f->addArg(mod->getByteptrType(), "data");
    f->finish();
}
