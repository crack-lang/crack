
#include "ext/Func.h"
#include "ext/Module.h"

using namespace crack::ext;

const char *echo(const char *data) { return data; }

extern "C" void test_testext_init(Module *mod) {
    Type *byteptrType = mod->getType(Module::byteptrType);
    Func *f = mod->addFunc(byteptrType, "echo", (void *)echo);
    f->addArg(byteptrType, "data");
    f->finish();
}
