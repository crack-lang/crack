// Copyright 2003 Michael A. Muller

#ifndef LOCATION_H
#define LOCATION_H

#include <spug/RCBase.h>
#include <spug/RCPtr.h>
#include <iostream>
#include <string>
#include <map>

namespace parser {

// forward declaration of Location map so we can befriend it.
class LocationMap;

class LocationImpl : public spug::RCBase {

   friend class LocationMap;
   private:
      std::string name;
      int lineNumber;

   public:

      LocationImpl(const char *name, int lineNumber) :
         name(name),
         lineNumber(lineNumber) {
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

      bool operator ==(const LocationImpl &other) const {
	 return name == other.name && lineNumber == other.lineNumber;
      }

      bool operator !=(const LocationImpl &other) const {
	 return !(*this == other);
      }

      friend std::ostream &
      operator <<(std::ostream &out, const LocationImpl &loc) {
	 return out << loc.name << ':' << std::dec << loc.lineNumber;
      }

};

SPUG_RCPTR(LocationImpl);

/**
 * Describes a location in source code.
 *
 * These are managed in the LocationMap
 */
class Location : public LocationImplPtr {
   friend class LocationMap;
   private:

   public:

      /**
       * You probably don't want to use this: use LocationMap::getLocation()
       * to keep them cached instead.
       */
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

      friend std::ostream &
      operator <<(std::ostream &out, const Location &loc) {
         return out << *loc;
      }
};


} // namespace parser

#endif

