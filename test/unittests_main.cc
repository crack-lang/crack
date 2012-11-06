// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include <stdint.h>
#include <sstream>
#include "model/GlobalNamespace.h"
#include "model/Serializer.h"
#include "model/Deserializer.h"
#include "model/ModuleDef.h"
#include "model/ModuleDefMap.h"
#include "model/TypeDef.h"

#include "tests/MockBuilder.h"
#include "tests/MockModuleDef.h"
#include "util/SourceDigest.h"

using namespace std;
using namespace model;
using namespace crack::util;

bool serializerTestUInt() {
    bool success = true;
    using namespace model;

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

struct DataSet {

    TypeDefPtr metaType, t0, t1;
    ModuleDefPtr builtins, dep0, dep1, mod;

    DataSet() {
        metaType =  new TypeDef(0, "Meta");
        metaType->type = metaType;
        builtins = new MockModuleDef("builtins", 0);
        builtins->addDef(metaType.get());

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
    ModuleDefMap deps;
    ds.t1->addDependenciesTo(ds.mod.get(), deps);

    if (deps.find("dep1") == deps.end()) {
        cerr << "dep1 not in module's deps" << endl;
        success = false;
    }

    if (deps.find("dep0") != deps.end()) {
        cerr << "indirect dependency is in module's deps" << endl;
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

bool moduleReload() {
    bool success = true;
    DataSet ds;

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

    istringstream src1(dep0Data.str());
    Deserializer deser1(src1, &context);
    ModuleDef::readHeaderAndVerify(deser1, SourceDigest());
    ModuleDefPtr dep0 = ModuleDef::deserialize(deser1, "dep0");
    construct.registerModule(dep0.get());

    istringstream src2(dep1Data.str());
    Deserializer deser2(src2, &context);
    ModuleDef::readHeaderAndVerify(deser2, SourceDigest());
    ModuleDefPtr dep1 = ModuleDef::deserialize(deser2, "dep1");
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

struct TestCase {
    const char *text;
    bool (*f)();
};

TestCase testCases[] = {
    {"serializerTestUInt", serializerTestUInt},
    {"moduleTestDeps", moduleTestDeps},
    {"moduleSerialization", moduleSerialization},
    {"moduleReload", moduleReload},
    {0, 0}
};

int main(int argc, const char **argv) {
    for (TestCase *test = testCases; test->text; ++test) {
        cerr << test->text << "..." << flush;
        cerr << (test->f() ? "ok" : "FAILED") << endl;
    }
}
