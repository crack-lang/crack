// Copyright 2010-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include <string.h>
#include "ext/Func.h"
#include "ext/Module.h"
#include "ext/Type.h"
#include "ext/util.h"

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
        
        static void init(MyType *inst, int a, const char *b) {
            inst->a = a;
            inst->b = b;
        }

        static const char *echo(MyType *inst, const char *data) { return data; }
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
    cout << "in testext" << endl;
}

// a class with virtual functions
struct MyVirtual {
    int *delFlag;
    MyVirtual(int *delFlag = 0) : delFlag(delFlag) {}
    virtual ~MyVirtual() {
        if (delFlag)
            *delFlag = 999;
    }
    virtual int vfunc(int val) {
        return val;
    }
    
    static int statFunc() {
        return 369;
    }
};

struct MyVirtual_Proxy;

// its adapter class
struct MyVirtual_Adapter : public MyVirtual {

    // translation functions for virtuals
    virtual int vfunc(int val);
    virtual ~MyVirtual_Adapter();

    // static wrapper functions    
    static int callVFunc(MyVirtual *inst, int val)  {
        return inst->MyVirtual::vfunc(val);
    }
    static void *operator new(size_t size, void *mem) { return mem; }
    static void init(void *inst) {
        new(inst) MyVirtual();
    }
    
    static void init1(void *inst, int *delFlag) {
        new(inst) MyVirtual(delFlag);
    }
    
    static void operator delete(void *mem) {}
    static void del(MyVirtual_Adapter *inst) {
        inst->MyVirtual::~MyVirtual();
    }
};

struct MyVirtual_VTable {
    void *classInst;
    int (*vfunc)(MyVirtual_Adapter *inst, int val);
    int (*del)(MyVirtual_Adapter *inst);
};

struct MyVirtual_Proxy {
    MyVirtual_VTable *vtable;
    MyVirtual_Adapter adapter;
};

// adapter call to dispatch to the Crack vtable
int MyVirtual_Adapter::vfunc(int val) {
    MyVirtual_Proxy *inst = getCrackProxy<MyVirtual_Proxy>(this);
    return inst->vtable->vfunc(this, val);
}

MyVirtual_Adapter::~MyVirtual_Adapter() {
    MyVirtual_Proxy *inst = getCrackProxy<MyVirtual_Proxy>(this);
    inst->vtable->del(this);
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
    f = type->addConstructor();
    f = type->addConstructor("init", (void *)MyType::init);
    f->addArg(mod->getIntType(), "a");
    f->addArg(mod->getByteptrType(), "b");
    type->finish();
    
    mod->addConstant(mod->getIntType(), "INT_CONST", 123);
    mod->addConstant(mod->getFloatType(), "FLOAT_CONST", 1.23);

    Type *arrayType = mod->getType("array");
    vector<Type *> params(1);
    params[0] = mod->getIntType();
    Type *intArrayType = arrayType->getSpecialization(params);

    if (intArrayType->toString() != "array[int]")
        cout << "FAILED constructing simple type string for array" << endl;

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

    // create a type with virtual methods.  We create a hidden type to 
    // strictly correspond to the instance area of the underlying type, then 
    // derive our proxy type from VTableBase and our hidden type.
    
    type = mod->addType("MyVirtual", sizeof(MyVirtual_Adapter),
                        true // hasVTable - necessary for virtual types!
                        );
    
    // we need some constructors to call the C++ class's constructors
    type->addConstructor("oper init", (void *)MyVirtual_Adapter::init);
    f = type->addConstructor("oper init", (void *)MyVirtual_Adapter::init1);
    f->addArg(intArrayType, "delFlag");
    
    // the order in which we add virtual methods must correspond to the order 
    // that they are defined in MyVirtual_VTable.
    f = type->addMethod(mod->getIntType(), "vfunc",
                        (void *)MyVirtual_Adapter::callVFunc
                        );
    f->addArg(mod->getIntType(), "val");
    f->setVWrap(true);

    // need a virtual destructor
    f = type->addMethod(mod->getVoidType(), "oper del",
                        (void *)MyVirtual_Adapter::del
                        );
    f->setVWrap(true);

    // adding a static method just to test that we can call it from another 
    // method defined inline.
    f = type->addStaticMethod(mod->getIntType(), "statFunc",
                              (void *)MyVirtual::statFunc
                              );

    // finish the type
    type->finish();
    Type *typeMyVirtual = type;

    // verify that we can create a class that calls static methods of a plain 
    // class and a virtual class.
    type = mod->addType("Caller", 0);
    type->addConstructor();
    type->addMethod(mod->getIntType(), "callMyTest",
                    "return MyType().a;"
                    );
    type->addMethod(typeMyVirtual, "createMyVirtual",
                    "return MyVirtual();"
                    );
    type->addMethod(mod->getIntType(), "statFunc",
                    "return 369; //return MyVirtual.statFunc();"
                    );
    type->finish();
}
