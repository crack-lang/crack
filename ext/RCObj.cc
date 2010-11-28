// Copyright 2010 Google Inc.

#include "RCObj.h"

using namespace crack::ext;

RCObj::~RCObj() {}
void RCObj::bind() { if (this) ++refCount; }
void RCObj::release() { if (this && !--refCount) delete this; }