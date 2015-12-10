// Copyright 2015 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
#ifndef _model_BaseMetaClass_h_
#define _model_BaseMetaClass_h_

namespace model {

class Context;

/**
 * Defines the methods and attributes of the base meta class (the "Class"
 * class in the language).  'context' should be the context of the .builtin
 * module.
 */
void populateBaseMetaClass(Context &context);

} // namespace model

#endif
