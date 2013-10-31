// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include <stdint.h>
#include <sstream>
#include <string.h>

#include "model/Generic.h"
#include "model/GlobalNamespace.h"
#include "model/Serializer.h"
#include "model/Deserializer.h"
#include "model/ModuleDef.h"
#include "model/ModuleDefMap.h"
#include "model/OverloadDef.h"
#include "model/TypeDef.h"
#include "parser/Toker.h"

#include "tests/MockBuilder.h"
#include "tests/MockFuncDef.h"
#include "tests/MockModuleDef.h"

using namespace parser;
using namespace std;
using namespace model;
using namespace crack::util;

bool serializerTestUInt() {
    bool success = true;

    ostringstream dst;
    Serializer s(dst);

    s.write(257, "data");
    string data = dst.str();
    istringstream src(data);

    uint8_t b = static_cast<unsigned>(data[0] & 0xFF);
    if (b != 0x81) {
        cerr << "first byte of data is " << (int)b << endl;
        success = false;
    }

    b = static_cast<unsigned>(data[1] & 0xff);
    if (data[1] != 2) {
        cerr << "second byte of data is " << (int)b << endl;
        success = false;
    }

    Deserializer d(src);
    int val = d.readUInt("data");
    if (val != 257) {
        cerr << "write/read of uint failed, got " << val << endl;
        success = false;
    }
    return success;
}

bool serializerTestLargeStrings() {
    ostringstream dst;
    Serializer s(dst);
    string testString = "large string - greater than 16 bytes";
    s.write(testString, "long_string");

    string data = dst.str();
    istringstream src(data);
    Deserializer d(src);
    if (d.readString(16, "long_string") != testString) {
        cerr << "reading large string back failed, got " << testString << endl;
        return false;
    } else {
        return true;
    }
}

struct DataSet {

    TypeDefPtr metaType, voidType, t0, t1;
    ModuleDefPtr builtins, dep0, dep1, mod;

    DataSet() {
        metaType = new TypeDef(0, "Meta");
        metaType->type = metaType;
        builtins = new MockModuleDef(".builtins", 0);
        builtins->addDef(metaType.get());
        voidType = new TypeDef(metaType.get(), "void");
        builtins->addDef(voidType.get());
        builtins->finished = true;
    }

    void addTestModules() {
        dep0 = new MockModuleDef("dep0", 0);
        t0 = new TypeDef(metaType.get(), "t0");
        dep0->addDef(t0.get());
        dep0->finished = true;

        dep1 = new MockModuleDef("dep1", 0);
        t1 = new TypeDef(metaType.get(), "t1");
        dep1->addDef(t1.get());
        dep1->addAlias(t0.get());
        OverloadDefPtr ovld = new OverloadDef("func");
        FuncDefPtr f = new MockFuncDef(FuncDef::noFlags, "func", 1);
        f->args[0] = new ArgDef(t1.get(), "a");
        f->returnType = voidType;
        ovld->addFunc(f.get());
        f->setOwner(dep1.get());
        f = new MockFuncDef(FuncDef::noFlags, "func", 1);
        f->args[0] = new ArgDef(t0.get(), "x");
        f->returnType = t0;
        ovld->addFunc(f.get());
        f->setOwner(dep1.get());
        dep1->addDef(ovld.get());
        dep1->finished = true;

        mod = new MockModuleDef("outer", 0);
        mod->addAlias(t1.get());
        mod->addAlias(ovld.get());
        mod->finished = true;
    }
};

bool moduleTestDeps() {
    DataSet ds;
    ds.addTestModules();
    bool success = true;
    VarDef::Set added;
    ds.t1->addDependenciesTo(ds.mod.get(), added);

    if (!ds.mod->dependencies.hasKey("dep1")) {
        cerr << "dep1 not in module's deps" << endl;
        success = false;
    }

    if (ds.mod->dependencies.hasKey("dep0")) {
        cerr << "indirect dependency is in module's deps" << endl;
        success = false;
    }
    return success;
}

bool moduleSerialization() {
    DataSet ds;
    ds.addTestModules();
    ostringstream out;
    Serializer ser(out);
    ds.mod->serialize(ser);
    Serializer ser2(out);
    ds.dep1->serialize(ser2);
    return true;
}

bool moduleReload() {
    bool success = true;
    DataSet ds;
    ds.addTestModules();

    ostringstream dep0Data, dep1Data, modData;
    Serializer ser1(dep0Data);
    ds.dep0->serialize(ser1);
    Serializer ser2(dep1Data);
    ds.dep1->serialize(ser2);
    Serializer ser3(modData);
    ds.mod->serialize(ser3);

    MockBuilder builder;
    builder.incref();
    builder.options = new builder::BuilderOptions();
    Construct construct(Options(), &builder);
    Context context(builder, Context::module, &construct,
                    0, // namespace, filled in by ModuleDef::deserialize()
                    new GlobalNamespace(0, "")
                    );
    context.incref();

    DataSet ds2;
    construct.registerModule(ds2.builtins.get());

    istringstream src1(dep0Data.str());
    Deserializer deser1(src1, &context);
    ModuleDefPtr dep0 = ModuleDef::deserialize(deser1, "dep0");
    dep0->finished = true;
    construct.registerModule(dep0.get());

    istringstream src2(dep1Data.str());
    Deserializer deser2(src2, &context);
    ModuleDefPtr dep1 = ModuleDef::deserialize(deser2, "dep1");
    dep1->finished = true;
    construct.registerModule(dep1.get());

    TypeDefPtr t = ds.mod->lookUp("t1");
    if (!t) {
        cerr << "Unable to lookup type t1 in mod" << endl;
        success = false;
    }
    if (t->getOwner() != ds.dep1.get()) {
        cerr << "Invalid owner of dep1.t1" << endl;
        success = false;
    }

    return success;
}

bool reloadOfSelfReferrentTypes() {
    bool success = true;
    DataSet ds;
    ds.addTestModules();

    TypeDefPtr myType = new TypeDef(ds.metaType.get(), "MyType");
    ds.dep0->addDef(myType.get());

    // create "MyType MyType.func(MyType a)"
    OverloadDefPtr ovld = new OverloadDef("func");
    FuncDefPtr f = new MockFuncDef(FuncDef::noFlags, "func", 1);
    f->args[0] = new ArgDef(myType.get(), "a");
    f->returnType = ds.voidType;
    ovld->addFunc(f.get());
    f->setOwner(myType.get());
    myType->addDef(ovld.get());

    MockBuilder builder;
    builder.incref();
    builder.options = new builder::BuilderOptions();
    Construct construct(Options(), &builder);
    construct.classType = new TypeDef(0, "Class");
    Context context(builder, Context::module, &construct,
                    0, // namespace, filled in by ModuleDef::deserialize()
                    new GlobalNamespace(0, "")
                    );
    context.incref();

    DataSet ds2;
    construct.registerModule(ds2.builtins.get());

    ostringstream data;
    Serializer ser(data);
    ds.dep0->serialize(ser);

    istringstream src(data.str());
    Deserializer deser(src, &context);
    ModuleDefPtr dep0 = ModuleDef::deserialize(deser, "dep0");

    myType = dep0->lookUp("MyType");
    ovld = myType->lookUp("func");
    if (!ovld) {
        cerr << "unable to lookup overload func" << endl;
        success = false;
    } else {
        ArgVec args(1);
        args[0] = new ArgDef(myType.get(), "a");
        f = ovld->getSigMatch(args, true);

        if (!f)
            cerr << "unable to find matching overload" << endl;
        else if (f->args[0]->type != myType)
            cerr << "incorrect return type deserialized" << endl;
    }

    return success;
}

bool operatorSerialization() {
    bool success = true;
    string org = "@ & << | >> ^ $ = &= *= <<= |= >>= ^= -= %= += /= * ! "
                 ": , -- := . == >= > ++ [ { <= ( < - != % + ? ] } ) ; "
                 "/ ~ && ||";
    istringstream src(org);
    Toker toker(src, "source-name");

    // serialize all of the tokens and build a list of them
    vector<Token> tokens;
    ostringstream serStr;
    Serializer ser(serStr);
    Token tok;
    while ((tok = toker.getToken()).getType() != Token::end) {
        Generic::serializeToken(ser, tok);
        tokens.push_back(tok);
    }

    // deserialize them and verify that the strings are all correct
    istringstream deserStr(serStr.str());
    Deserializer deser(deserStr, 0);
    for (int i = 0; i < tokens.size(); ++i) {
        Token tok = Generic::deserializeToken(deser);
        if (tok.getData() != tokens[i].getData()) {
            cerr << "mismatch reading back " << tokens[i].getData() <<
                " got " << tok.getData() << endl;
            success = false;
        }
    }
    return success;
}

struct TestCase {
    const char *text;
    bool (*f)();
};

TestCase testCases[] = {
    {"serializerTestUInt", serializerTestUInt},
    {"serializerTestLargeStrings", serializerTestLargeStrings},
    {"moduleTestDeps", moduleTestDeps},
    {"moduleSerialization", moduleSerialization},
    {"moduleReload", moduleReload},
    {"reloadOfSelfReferrentTypes", reloadOfSelfReferrentTypes},
    {"operatorSerialization", operatorSerialization},
    {0, 0}
};

int main(int argc, const char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[1], "Serializer"))
            Serializer::trace = true;
    }

    for (TestCase *test = testCases; test->text; ++test) {
        cerr << test->text << "..." << flush;
        cerr << (test->f() ? "ok" : "FAILED") << endl;
    }
}
