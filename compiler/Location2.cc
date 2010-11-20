
#include "Location.h"

#include "parser/Location.h"

using namespace compiler;

Location::Location(const parser::Location &loc) {
    rep = new parser::Location(loc);
}

Location::~Location() {
    delete rep;
}

const char *Location::getName() {
    return rep->getName();
}

int Location::getLineNumber() {
    return rep->getLineNumber();
}

