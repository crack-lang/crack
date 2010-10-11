
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"

#include <iostream>

using namespace crack::ext;
using namespace std;

class MyType {};

const char *echo(MyType *inst, const char *data) { return data; }
MyType *MyType_oper_new() { return new MyType(); }

extern "C" void test_testext_init(Module *mod) {
    Func *f = mod->addFunc(mod->getByteptrType(), "echo", (void *)echo);
    f->addArg(mod->getByteptrType(), "data");
    
    Type *type = mod->addType("MyType");
    type->addStaticMethod(type, "oper new", (void *)MyType_oper_new);
    f = type->addMethod(mod->getByteptrType(), "echo", (void *)echo);
    f->addArg(mod->getByteptrType(), "data");
    type->finish();
}
