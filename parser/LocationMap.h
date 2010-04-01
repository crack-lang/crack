// Copyright 2003 Michael A. Muller

#ifndef LOCATIONMAP_H
#define LOCATIONMAP_H

#include <string>

namespace parser {

/**
 * Keeps track of a set of location objects.  This class also tracks the state
 * of an input stream - it has a "current file" and "current line" which it
 * uses to optimize lifecycle management of the location objects.
 */
class LocationMap {

   private:
      std::string name;
      int lineNumber;

   public:

      /** sets the current source name and optionally the line number */
      void setName(const char *newName, int newLineNumber = 1) {
	 name = newName;
	 lineNumber = newLineNumber;
      }

      /** sets the current source line number */
      void setLineNumber(int lineNumber) {
	 lineNumber = lineNumber;
      }

      /** returns a location object for the current location */
      Location getLocation() const {
	 return Location(name.c_str(), lineNumber);
      }

      /** increment the line number */
      void incrementLineNumber() {
	 ++lineNumber;
      }
      
      /** Decrement the line number (needed for character putbacks) */
      void decrementLineNumber() {
         --lineNumber;
      }
};

} // namespace parser

#endif
