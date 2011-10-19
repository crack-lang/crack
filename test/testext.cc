
#include <string.h>
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

        static const char *echo(MyType *inst, const char *data) { return data; }
};

class MyAggType : public Object {
    public:
        int a;

        static void init(MyAggType *inst, int a0) { inst->a = a0; }
        static void dump(MyAggType *inst) { 
            cout << "a = " << inst->a << endl; 
            char *c = (char *)inst;
            for (int i = 0; i < 32; ++i)
                cout << hex << ((int)c[i] & 0xff) << " ";
            cout << endl;
        }
};    

const char *echo(const char *data) { return data; }
extern "C" const char *cecho(const char *data) { return data; }

int *copyArray(int count, int *array) {
    int *result = new int[count];
    return (int *)memcpy(result, array, count * sizeof(int));
}

int callback(int (*cb)(int)) {
    return cb(100);
}

extern "C" void testext_rinit(void) {
}

extern "C" void testext_cinit(Module *mod) {
    Func *f = mod->addFunc(mod->getByteptrType(), "echo", (void *)echo);
    f->addArg(mod->getByteptrType(), "data");
    
    Type *type = mod->addType("MyType", sizeof(MyType));
    type->addInstVar(mod->getIntType(), "a", CRACK_OFFSET(MyType, a));
    type->addInstVar(mod->getByteptrType(), "b", CRACK_OFFSET(MyType, b));

    type->addStaticMethod(type, "oper new", (void *)MyType::oper_new);
    f = type->addMethod(mod->getByteptrType(), "echo", (void *)MyType::echo);
    f->addArg(mod->getByteptrType(), "data");
    type->finish();
    
    type = mod->addType("MyAggType", sizeof(MyType) - sizeof(Object));
    type->addBase(mod->getObjectType());
    type->addInstVar(mod->getIntType(), "a", CRACK_OFFSET(MyAggType, a)); 
    f = type->addConstructor();
    f = type->addConstructor("init", (void *)MyAggType::init);
    f->addArg(mod->getIntType(), "a");
    type->addMethod(mod->getVoidType(), "dump", (void *)MyAggType::dump);
    type->finish();

    mod->addConstant(mod->getIntType(), "INT_CONST", 123);
    mod->addConstant(mod->getFloatType(), "FLOAT_CONST", 1.23);

    Type *arrayType = mod->getType("array");
    vector<Type *> params(1);
    params[0] = mod->getIntType();
    Type *intArrayType = arrayType->getSpecialization(params);
    f = mod->addFunc(intArrayType, "copyArray", (void *)copyArray);
    f->addArg(mod->getIntType(), "count");
    f->addArg(intArrayType, "array");

    Type *funcType = mod->getType("function");
    vector<Type *> fparams(2);
    fparams[0] = mod->getIntType(); // return type
    fparams[1] = mod->getIntType(); // 1st param
    Type *intFuncType = funcType->getSpecialization(fparams);
    f = mod->addFunc(mod->getIntType(), "callback", (void *)callback);
    f->addArg(intFuncType, "cb");

}
