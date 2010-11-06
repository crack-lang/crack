
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/Object.h"

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

class MyAggType : public Object {
    public:
        int a;

        void init(int a0) { a = a0; }
        void dump() { 
            cout << "a = " << a << endl; 
            char *c = (char *)this;
            for (int i = 0; i < 32; ++i)
                cout << hex << ((int)c[i] & 0xff) << " ";
            cout << endl;
        }
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
    
    type = mod->addType("MyAggType");
    type->addBase(mod->getObjectType());
    type->addInstVar(mod->getIntType(), "a"); 
    f = type->addConstructor();
    f = type->addConstructor("init", (void *)&MyAggType::init);
    f->addArg(mod->getIntType(), "a");
    type->addMethod(mod->getVoidType(), "dump", (void *)&MyAggType::dump);
    type->finish();

    mod->addConstant(mod->getIntType(), "INT_CONST", 123);
    mod->addConstant(mod->getFloatType(), "FLOAT_CONST", 1.23);                     
}
