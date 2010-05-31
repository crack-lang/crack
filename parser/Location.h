// Copyright 2003 Michael A. Muller

#ifndef LOCATION_H
#define LOCATION_H

#include <iostream>
#include <string>

namespace parser {

// forward declaration of Location map so we can befriend it.
class LocationMap;

/**
 * Describes a location in source code.
 *
 * These are managed in the LocationMap
 */
class Location {
   friend class LocationMap;
   private:
      std::string name;
      int lineNumber;

      Location(const char *name, int lineNumber);

   public:

      /** so that we can construct one of these prior to assigning it */
      Location();

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

      bool operator ==(const Location &other) const {
	 return name == other.name && lineNumber == other.lineNumber;
      }

      bool operator !=(const Location &other) const {
	 return !(*this == other);
      }

      friend std::ostream &
      operator <<(std::ostream &out, const Location &loc) {
	 return out << loc.name << ':' << std::dec << loc.lineNumber;
      }
};


} // namespace parser

#endif

