// Copyright 2010 Google Inc.

#include "RCObj.h"

using namespace crack::ext;

RCObj::~RCObj() {}
void RCObj::bind() { ++refCount; }
void RCObj::release() { if (!--refCount) delete this; }