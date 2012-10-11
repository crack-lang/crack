// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include <stdint.h>
#include <sstream>
#include "model/Serializer.h"
#include "model/Deserializer.h"
#include "model/ModuleDef.h"
#include "model/TypeDef.h"

using namespace std;
using namespace model;

bool serializerTestUInt() {
    bool success = true;
    using namespace model;

    ostringstream dst;
    Serializer s(dst);

    s.write(257);
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
    int val = d.readUInt();
    if (val != 257) {
        cerr << "write/read of uint failed, got " << val << endl;
        success = false;
    }
    return success;
}

struct MockModuleDef : public ModuleDef {

    MockModuleDef(const std::string &name, Namespace *parent) :
        ModuleDef(name, parent) {
    }

    virtual void callDestructor() {}
    virtual bool matchesSource(const string &path) { return false; }
};

struct DataSet {

    TypeDefPtr metaType, t0, t1;
    ModuleDefPtr dep0, dep1, mod;

    DataSet() {
        metaType =  new TypeDef(0, "Meta");
        metaType->type = metaType;

        dep0 = new MockModuleDef("dep0", 0);
        t0 = new TypeDef(metaType.get(), "t0");
        dep0->addDef(t0.get());

        dep1 = new MockModuleDef("dep1", 0);
        t1 = new TypeDef(metaType.get(), "t1");
        dep1->addDef(t1.get());
        dep1->addAlias(t0.get());

        mod = new MockModuleDef("outer", 0);
        mod->addAlias(t1.get());
    }
};

bool moduleTestDeps() {
    DataSet ds;
    bool success = true;
    set<string> deps;
    ds.mod->computeDependencies(deps);

    if (deps.find("dep1") == deps.end()) {
        cerr << "dep1 not in outer's deps" << endl;
        success = false;
    }

    if (deps.find("dep0") == deps.end()) {
        cerr << "dep0 not in outer's deps" << endl;
        success = false;
    }
    return success;
}

bool moduleSerialization() {
    DataSet ds;
    ostringstream out;
    Serializer ser(out);
    ds.mod->serialize(ser);
    ds.dep1->serialize(ser);
    return true;
}

struct TestCase {
    const char *text;
    bool (*f)();
};

TestCase testCases[] = {
    {"serializerTestUInt", serializerTestUInt},
    {"moduleTestDeps", moduleTestDeps},
    {"moduleSerialization", moduleSerialization},
    {0, 0}
};

int main(int argc, const char **argv) {
    for (TestCase *test = testCases; test->text; ++test) {
        cerr << test->text << "..." << flush;
        cerr << (test->f() ? "ok" : "FAILED") << endl;
    }
}
