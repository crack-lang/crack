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
      
      typedef std::pair<std::string, int> LocTuple;
      typedef std::map<LocTuple, Location> LocMap;
      LocMap locMap;
      Location lastLoc;

   public:

      /** sets the current source name and optionally the line number */
      void setName(const std::string &newName, int newLineNumber = 1) {
         LocMap::iterator item =
            locMap.find(LocTuple(newName, newLineNumber));
         if (item != locMap.end()) {
            lastLoc = item->second;
         } else {
            lastLoc = new LocationImpl(newName, newLineNumber);
         }            
	 name = newName;
	 lineNumber = newLineNumber;
      }

      /** sets the current source line number */
      void setLineNumber(int lineNumber) {
	 lineNumber = lineNumber;
      }

      /** returns a location object for the current location */
      Location getLocation() const {
         return lastLoc;
      }
      
      /** Returns a Location object for the specified location */
      Location getLocation(const char *name, int lineNumber) {
         setName(name, lineNumber);
         return lastLoc;
      }

      /** increment the line number */
      void incrementLineNumber() {
         setName(lastLoc->name.c_str(), lastLoc->lineNumber + 1);
      }
      
      /** Decrement the line number (needed for character putbacks) */
      void decrementLineNumber() {
         setName(lastLoc->name.c_str(), lastLoc->lineNumber - 1);
      }
};

} // namespace parser

#endif
