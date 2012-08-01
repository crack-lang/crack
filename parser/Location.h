// Copyright 2003 Michael A. Muller <mmuller@enduden.com>
// Copyright 2010,2012 Google Inc.
// Copyright 2012 Shannon Weyrick <weyrick@mozek.us>
// 
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
// 

#ifndef LOCATION_H
#define LOCATION_H

#include <spug/RCBase.h>
#include <spug/RCPtr.h>
#include <iostream>
#include <string>
#include <map>

namespace parser {

class LocationImpl : public spug::RCBase {

   private:
      std::string name;
      int lineNumber;
      int startCol;
      int endCol;

   public:

      LocationImpl(const std::string &name,
                   int lineNumber,
                   int startCol = 1,
                   int endCol = 1) :
         name(name),
         lineNumber(lineNumber),
         startCol(startCol),
         endCol(endCol) {

      }

      /** so that we can construct one of these prior to assigning it */
      LocationImpl();

      /**
       * Returns the file/stream name.  The name is guaranteed to be in
       * existence for as long as the instance is
       */
      const char *getName() const {
          return name.c_str();
      }

      /** returns the source line number */
      int getLineNumber() const {
          return lineNumber;
      }

      /** returns the source column numbers */
      int getColNumber() const {
          return startCol;
      }

      int getStartCol() const {
          return startCol;
      }

      int getEndCol() const {
          return endCol;
      }

      bool operator ==(const LocationImpl &other) const {
          return name == other.name &&
                 lineNumber == other.lineNumber &&
                 startCol == other.startCol &&
                 endCol == other.endCol;
      }

      bool operator !=(const LocationImpl &other) const {
          return !(*this == other);
      }

      friend std::ostream &
      operator <<(std::ostream &out, const LocationImpl &loc) {
          return out << loc.name << ':' << std::dec << loc.lineNumber <<
                 ":" << loc.startCol;
      }

};

SPUG_RCPTR(LocationImpl);

/**
 * Describes a location in source code.
 *
 * These are managed in the LocationMap
 */
class Location : public LocationImplPtr {

   private:

   public:

      Location(LocationImpl *impl) : LocationImplPtr(impl) {}

      /** so that we can construct one of these prior to assigning it */
      Location() {}

      /**
       * Returns the file/stream name.  The name is guaranteed to be in
       * existence for as long as the instance is
       */
      const char *getName() const {
         return get()->getName();
      }

      /** returns the source line number */
      int getLineNumber() const {
         return get()->getLineNumber();
      }

      /** returns the source columns */
      int getColNumber() const {
         return get()->getColNumber();
      }
      int getStartCol() const {
         return get()->getStartCol();
      }
      int getEndCol() const {
         return get()->getEndCol();
      }

      friend std::ostream &
      operator <<(std::ostream &out, const Location &loc) {
         return out << *loc;
      }
};


} // namespace parser

#endif

