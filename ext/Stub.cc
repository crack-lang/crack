// Copyright 2011-2012 Google Inc.
// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2011 Arno Rehn <arno@arnorehn.de>
// Copyright 2011-2012 Conrad Steenberg <conrad.steenberg@gmail.com>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Stub.h"

using namespace crack::ext;
using namespace std;

// Func
void Func::setInitializers(const std::string&) { }
std::string Func::getInitializers() const { return std::string(); }
unsigned int Func::getVTableOffset() const { return 0; }
void Func::setBody(const std::string&) { }
std::string Func::getBody() const { return std::string(); }
void Func::setIsVariadic(bool isVariadic) { }
bool Func::isVariadic() const { return false; }
void Func::setVWrap(bool vwrapEnabled) { }
bool Func::getVWrap() const { return false; }
void Func::setVirtual(bool virtualizedEnabled) {}
bool Func::getVirtual() const { return false; }
void Func::setSymbolName(const std::string &name) { }
void Func::addArg(Type *type, const std::string &name) { }
void Func::finish() { }

// Module
Module::Module(model::Context *context) { }
Type *Module::getClassType() { }
Type *Module::getVoidType() { }
Type *Module::getVoidptrType() { }
Type *Module::getBoolType() { }
Type *Module::getByteptrType() { }
Type *Module::getByteType() { }
Type *Module::getInt16Type() { }
Type *Module::getInt32Type() { }
Type *Module::getInt64Type() { }
Type *Module::getUint16Type() { }
Type *Module::getUint32Type() { }
Type *Module::getUint64Type() { }
Type *Module::getIntType() { }
Type *Module::getUintType() { }
Type *Module::getIntzType() { }
Type *Module::getUintzType() { }
Type *Module::getFloat32Type() { }
Type *Module::getFloat64Type() { }
Type *Module::getFloatType() { }
Type *Module::getVTableBaseType() { }
Type *Module::getObjectType() { }
Type *Module::getStringType() { }
Type *Module::getStaticStringType() { }
Type *Module::getOverloadType() { }

Type *Module::getType(const char *name) { }
Type *Module::addType(const char *name, size_t instSize, bool hasVTable) { }
Type *Module::addForwardType(const char *name, size_t instSize) { }
Func *Module::addFunc(Type *returnType, const char *name, void *funcPtr,
                      const char *symbolName) { }
Func *Module::addFunc(Type *returnType, const char *name,
                      const std::string& body) { }
void Module::addConstant(Type *type, const std::string &name, double val)  { }
void Module::addConstant(Type *type, const std::string &name, int64_t val)  { }
void Module::addConstant(Type *type, const std::string &name, int val)  { }
void Module::inject(const std::string&) { }
void Module::finish()  { }

// Type
void Type::checkFinished() { }
void Type::addBase(Type *base) { }
void Type::addInstVar(Type *type, const std::string &name, size_t offset) { }

Func *Type::addMethod(Type *returnType, const std::string &name,
                void *funcPtr
                ) { };
Func *Type::addMethod(Type *returnType, const std::string &name,
                      const std::string& body
                      ) { }

Func *Type::addConstructor(const char *name, void *funcPtr) { };
Func *Type::addConstructor(const std::string& body) { }

Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                      void *funcPtr
                      ) { };
Func *Type::addStaticMethod(Type *returnType, const std::string &name,
                      const std::string& body
                      ) { }
const vector<Func *>& Type::getMethods() const { };
bool Type::methodHidesOverload(const string& name,
                               const vector<Type *>& args) const { };
vector<Type *> Type::getGenericParams() const { };
Type *Type::getSpecialization(const vector<Type *> &params) { };
bool Type::isPrimitive() const { };
string Type::toString() const { };
void Type::injectBegin(const std::string& code) { };
void Type::injectEnd(const std::string& code) { };

void Type::finish() { };
