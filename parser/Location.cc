
#include "Location.h"

Location::Location() :
   name(""),
   lineNumber(0) {
}

Location::Location(const char *name, int lineNumber) :
   name(name),
   lineNumber(lineNumber) {
}

