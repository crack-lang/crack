// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2010 Google Inc.
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#include "Location.h"

Location::Location() :
   name(""),
   lineNumber(0) {
}

Location::Location(const char *name, int lineNumber) :
   name(name),
   lineNumber(lineNumber) {
}

