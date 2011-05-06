// copyright 2011 Google Inc.

#include "Generic.h"

#include "parser/Toker.h"

using namespace model;

GenericParm *Generic::getParm(const std::string &name) {
    for (int i = 0; i < parms.size(); ++i)
        if (parms[i]->name == name)
            return parms[i].get();
    
    return 0;
}

void Generic::replay(parser::Toker &toker) {
    // we have to put back the token list in reverse order.
    for (int i = body.size() - 1; i >= 0; --i)
        toker.putBack(body[i]);
}
