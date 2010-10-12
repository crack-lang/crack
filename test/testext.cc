
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"

#include <iostream>

using namespace crack::ext;
using namespace std;

class MyType {
    public:
        int a;
        const char *b;
        
        MyType(int a, const char *b) : a(a), b(b) {}

        static MyType *oper_new() {
            return new MyType(100, "test");
        }

        const char *echo(const char *data) { return data; }
};

const char *echo(const char *data) { return data; }

extern "C" void test_testext_init(Module *mod) {
    Func *f = mod->addFunc(mod->getByteptrType(), "echo", (void *)echo);
    f->addArg(mod->getByteptrType(), "data");
    
    Type *type = mod->addType("MyType");
    type->addInstVar(mod->getIntType(), "a");
    type->addInstVar(mod->getByteptrType(), "b");

    type->addStaticMethod(type, "oper new", (void *)MyType::oper_new);
    f = type->addMethod(mod->getByteptrType(), "echo", (void *)&MyType::echo);
    f->addArg(mod->getByteptrType(), "data");
    type->finish();
}
