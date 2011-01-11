
#include "Location.h"

#include "parser/Location.h"

using namespace compiler;
using namespace crack::ext;

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

const char *Location::_getName(Location *inst) {
    return inst->rep->getName();
}

int Location::_getLineNumber(Location *inst) {
    return inst->rep->getLineNumber();
}

void Location::_bind(Location *inst) { inst->bind(); }
void Location::_release(Location *inst) { inst->release(); }

