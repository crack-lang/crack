// Copyright 2013 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#ifndef _spug_stlutil_h_
#define _spug_stlutil_h_

// Macro to simplify iteration over a collection.  This uses a const iterator,
// for mutating iteration use SPUG_MFOR.
#define SPUG_MFOR(collection_type, iter, collection) \
    for (typename collection_type::iterator iter = (collection).begin(); \
         iter != (collection).end(); \
         ++iter \
         )

namespace spug {

// "contains" function.  More intuitive way to see if an element is in a
// collection.
template <class Coll, class Elem>
inline bool contains(Coll coll, Elem elem) {
    return coll.find(elem) != coll.end();
}

// Macro to simplify iteration over a collection.  This uses a const iterator,
// for mutating iteration use SPUG_MFOR.
// This is a very weak version of boost's foreach.  The actual implementation
// is too complicated and has too many boost dependencies to just import into
// the tree.
#define SPUG_FOR(collection_type, iter, collection) \
    for (collection_type::const_iterator iter = (collection).begin(); \
         iter != (collection).end(); \
         ++iter)

} // namespace spug

#endif
